#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <openssl/ssl.h>

#include "reactor.h"
#include "data.h"
#include "descriptor.h"
#include "stream.h"

static __thread int stream_ssl_activated = 0;

/* stream socket/pipe operations */


#include <err.h>
static size_t stream_socket_read(stream *stream)
{
  ssize_t n;
  size_t total;

  total = 0;
  do
  {
    buffer_reserve(&stream->input, buffer_size(&stream->input) + STREAM_RECEIVE_SIZE);
    n = read(descriptor_fd(&stream->descriptor), (uint8_t *) buffer_data(&stream->input) + buffer_size(&stream->input), STREAM_RECEIVE_SIZE);
    if (n == -1)
      break;
    total += n;
    stream->input.size += n;
  } while (n == STREAM_RECEIVE_SIZE);
  return total;
}

static int stream_socket_mask(stream *stream)
{
  return (stream->mask & STREAM_READ) | (buffer_size(&stream->output) || stream->notify ? STREAM_WRITE : 0);
}

static void stream_socket_update(stream *stream)
{
  descriptor_mask(&stream->descriptor, stream_socket_mask(stream));
}

static void stream_socket_write(stream *stream)
{
  char *data = buffer_data(&stream->output);
  size_t size = buffer_size(&stream->output);
  ssize_t n;
  size_t total;

  total = 0;
  while (size - total)
  {
    n = write(descriptor_fd(&stream->descriptor), data + total, size - total);
    if (n == -1)
    {
      assert(errno == EAGAIN);
      break;
    }
    total += n;
  }
  buffer_erase(&stream->output, 0, total);
}

static void stream_socket_callback(reactor_event *event)
{
  stream *stream = event->state;
  size_t n;

  switch (event->type)
  {
  case DESCRIPTOR_READ:
    n = stream_socket_read(stream);
    if (n)
      reactor_dispatch(&stream->handler, STREAM_READ, 0);
    break;
  case DESCRIPTOR_WRITE:
    stream_socket_write(stream);
    if (buffer_size(&stream->output) == 0 || stream->notify)
    {
      stream->notify = 0;
      stream_socket_update(stream);
      reactor_dispatch(&stream->handler, STREAM_WRITE, 0);
      break;
    }  
    stream_socket_update(stream);
    break;
  default:
  case DESCRIPTOR_CLOSE:
    reactor_dispatch(&stream->handler, STREAM_CLOSE, 0);
    break;
  }
}

static void stream_socket_open(stream *stream, int fd)
{
  descriptor_construct(&stream->descriptor, stream_socket_callback, stream);
  descriptor_open(&stream->descriptor, fd, stream_socket_mask(stream));
}

/* stream ssl operations */

static void stream_ssl_update(stream *stream)
{
  assert(stream_ssl_activated);

  descriptor_mask(&stream->descriptor,
                  DESCRIPTOR_READ |
                  ((stream->ssl_state == SSL_ERROR_WANT_WRITE) | (buffer_size(&stream->output) > 0) ? DESCRIPTOR_WRITE : 0));
}

static size_t stream_ssl_read(stream *stream)
{
  ssize_t n;
  size_t total;

  assert(stream_ssl_activated);
  total = 0;
  while (1)
  {
    buffer_reserve(&stream->input, buffer_size(&stream->input) + 16384);
    n = SSL_read(stream->ssl, (uint8_t *) buffer_data(&stream->input) + buffer_size(&stream->input), 16384);
    if (n <= 0)
      break;
    total += n;
    stream->input.size += n;
  }
  stream->ssl_state = SSL_get_error(stream->ssl, n);
  return total;
}

static void stream_ssl_flush(stream *stream)
{
  ssize_t n;

  assert(stream_ssl_activated);
  n = SSL_write(stream->ssl, buffer_data(&stream->output), buffer_size(&stream->output));
  if (n > 0)
    buffer_erase(&stream->output, 0, n);
  stream->ssl_state = SSL_get_error(stream->ssl, n);
}

static void stream_ssl_callback(reactor_event *event)
{
  stream *stream = event->state;
  size_t n;

  switch (event->type)
  {
  case DESCRIPTOR_READ:
    n = stream_ssl_read(stream);
    stream_ssl_update(stream);
    if (n)
      reactor_dispatch(&stream->handler, STREAM_READ, 0);
    break;
  case DESCRIPTOR_WRITE:
    stream_ssl_flush(stream);
    if (buffer_size(&stream->output) == 0 && stream->notify)
    {
      stream->notify = 0;
      stream_ssl_update(stream);
      reactor_dispatch(&stream->handler, STREAM_WRITE, 0);
      break;
    }  
    stream_ssl_update(stream);
    break;
  default:
  case DESCRIPTOR_CLOSE:
    reactor_dispatch(&stream->handler, STREAM_CLOSE, 0);
    break;
  }
}

static void stream_ssl_open(stream *stream, int fd, SSL_CTX *ssl_ctx)
{
  stream_ssl_activated = 1;

  assert(stream->is_socket);
  stream->ssl = SSL_new(ssl_ctx);
  assert(stream->ssl != NULL);
  SSL_set_fd(stream->ssl, fd);
  if (SSL_is_server(stream->ssl))
    SSL_set_accept_state(stream->ssl);
  else
    SSL_set_connect_state(stream->ssl);
  SSL_set_mode(stream->ssl, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
  descriptor_construct(&stream->descriptor, stream_ssl_callback, stream);
  descriptor_open(&stream->descriptor, fd, DESCRIPTOR_WRITE);
}

/* stream */

void stream_construct(stream *stream, reactor_callback *callback, void *state)
{
  *stream = (struct stream) {0};
  reactor_handler_construct(&stream->handler, callback, state);
  descriptor_construct(&stream->descriptor, NULL, NULL);
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
}

void stream_destruct(stream *stream)
{
  stream_close(stream);
  buffer_destruct(&stream->input);
  buffer_destruct(&stream->output);
  descriptor_destruct(&stream->descriptor);
  reactor_handler_destruct(&stream->handler);
}

void stream_open(stream *stream, int fd, int mask, SSL_CTX *ssl_ctx)
{
  struct stat st;
  int e;

  e = fstat(fd, &st);
  assert(e == 0);
  stream->is_socket = S_ISSOCK(st.st_mode);
  stream->mask = mask;
  if (ssl_ctx)
    stream_ssl_open(stream, fd, ssl_ctx);
  else
    stream_socket_open(stream, fd);
}

void stream_notify(stream *stream)
{
  stream->notify = 1;
  if (!descriptor_active(&stream->descriptor))
    return;
  if (stream->ssl)
    stream_ssl_update(stream);
  else
    stream_socket_update(stream);
}

void stream_close(stream *stream)
{
  if (!descriptor_active(&stream->descriptor))
    return;
  
  if (stream_ssl_activated)
    SSL_free(stream->ssl);
  
  stream->notify = 0;
  descriptor_close(&stream->descriptor);
  buffer_clear(&stream->input);
  buffer_clear(&stream->output);
}

data stream_read(stream *stream)
{
  return data_construct(buffer_data(&stream->input), buffer_size(&stream->input));
}

void stream_consume(stream *stream, size_t size)
{
  buffer_erase(&stream->input, 0, size);
}

void stream_write(stream *stream, data data)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), (void *) data_base(data), data_size(data));
}

void *stream_allocate(stream *stream, size_t size)
{
  size_t current_size = buffer_size(&stream->output);

  buffer_resize(&stream->output, current_size + size);
  return (char *) buffer_data(&stream->output) + current_size;
}

void stream_flush(stream *stream)
{
  if (stream->ssl)
  {
    stream_ssl_flush(stream);
    stream_ssl_update(stream);
  }
  else
  { stream_socket_write(stream);
    stream_socket_update(stream);
  }
}
