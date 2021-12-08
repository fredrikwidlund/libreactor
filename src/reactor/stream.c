#include <stdio.h>
#include <sys/socket.h>
#include <openssl/ssl.h>

#include "reactor.h"
#include "descriptor.h"
#include "stream.h"

/* stream socket operations */

static void stream_socket_open(stream *stream, int fd, SSL_CTX *ssl_ctx)
{
  (void) ssl_ctx;
  descriptor_open(&stream->descriptor, fd, stream->write_notify);
}

static void stream_socket_read(stream *stream)
{
  ssize_t n;

  do
  {
    buffer_reserve(&stream->input, buffer_size(&stream->input) + STREAM_RECEIVE_SIZE);
    n = recv(descriptor_fd(&stream->descriptor), (uint8_t *) buffer_data(&stream->input) + buffer_size(&stream->input),
             STREAM_RECEIVE_SIZE, MSG_DONTWAIT);
    if (n <= 0)
      break;
    stream->input.size += n;
  } while (n == STREAM_RECEIVE_SIZE);
  stream->errors += n == 0;
}

static void stream_socket_write(stream *stream)
{
  char *data = buffer_data(&stream->output);
  size_t size = buffer_size(&stream->output);
  ssize_t n;

  while (size)
  {
    n = send(descriptor_fd(&stream->descriptor), data, size, MSG_DONTWAIT);
    if (n == -1)
      break;
    data += n;
    size -= n;
  }
  buffer_erase(&stream->output, 0, buffer_size(&stream->output) - size);
  descriptor_write_notify(&stream->descriptor, buffer_size(&stream->output) > 0);
}

/* stream ssl operations */

static void stream_ssl_open(stream *stream, int fd, SSL_CTX *ssl_ctx)
{
  stream->ssl = SSL_new(ssl_ctx);
  SSL_set_fd(stream->ssl, fd);
  (SSL_is_server(stream->ssl) ? SSL_set_accept_state : SSL_set_connect_state)(stream->ssl);
  descriptor_open(&stream->descriptor, fd, stream->write_notify);
}

static void stream_ssl_update(stream *stream, int return_code)
{
  stream->ssl_state = SSL_get_error(stream->ssl, return_code);
  descriptor_write_notify(&stream->descriptor, (buffer_size(&stream->output) > 0) | (stream->ssl_state == SSL_ERROR_WANT_WRITE));
  stream->errors += (stream->ssl_state > SSL_ERROR_WANT_WRITE) | (stream->ssl_state == SSL_ERROR_SSL);
}

static void stream_ssl_read(stream *stream)
{
  int n, total = 0;

  while (1)
  {
    buffer_reserve(&stream->input, buffer_size(&stream->input) + 16384);
    n = SSL_read(stream->ssl, (uint8_t *) buffer_data(&stream->input) + buffer_size(&stream->input), 16384);
    if (n <= 0)
      break;
    stream->input.size += n;
    total += n;
  }
  stream_ssl_update(stream, n);
}

static void stream_ssl_write(stream *stream)
{
  ssize_t n;

  if ((buffer_size(&stream->output) > 0) | (stream->ssl_state == SSL_ERROR_WANT_WRITE))
  {
    n = SSL_write(stream->ssl, buffer_data(&stream->output), buffer_size(&stream->output));
    if (n > 0)
      buffer_erase(&stream->output, 0, n);
    stream_ssl_update(stream, n);
  }
}

/* stream */

static void stream_callback(reactor_event *event)
{
  stream *stream = event->state;

  switch (event->type)
  {
  case DESCRIPTOR_READ:
    (stream->ssl ? stream_ssl_read : stream_socket_read)(stream);
    reactor_dispatch(&stream->handler, stream->errors ? STREAM_CLOSE : STREAM_READ, 0);
    break;
  case DESCRIPTOR_WRITE:
    stream_flush(stream);
    if (buffer_size(&stream->output) == 0 && stream->write_notify)
    {
      stream->write_notify = 0;
      reactor_dispatch(&stream->handler, STREAM_WRITE, 0);
    }
    break;
  default:
  case DESCRIPTOR_CLOSE:
    reactor_dispatch(&stream->handler, STREAM_CLOSE, 0);
    break;
  }
}

void stream_construct(stream *stream, reactor_callback *callback, void *state)
{
  *stream = (struct stream) {0};
  reactor_handler_construct(&stream->handler, callback, state);
  descriptor_construct(&stream->descriptor, stream_callback, stream);
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

void stream_open(stream *stream, int fd, SSL_CTX *ssl_ctx, int write_notify)
{
  stream->write_notify = write_notify;
  (ssl_ctx ? stream_ssl_open : stream_socket_open)(stream, fd, ssl_ctx);
}

void stream_close(stream *stream)
{
  if (descriptor_fd(&stream->descriptor) >= 0)
  {
    SSL_free(stream->ssl);
    descriptor_close(&stream->descriptor);
    buffer_clear(&stream->input);
    buffer_clear(&stream->output);
    stream->write_notify = 0;
    stream->errors = 0;
    stream->ssl = NULL;
    stream->ssl_state = 0;
  }
}

void stream_write_notify(stream *stream)
{
  stream->write_notify = 1;
  descriptor_write_notify(&stream->descriptor, 1);
}

void stream_read(stream *stream, void **base, size_t *size)
{
  *base = buffer_data(&stream->input);
  *size = buffer_size(&stream->input);
}

void stream_consume(stream *stream, size_t size)
{
  buffer_erase(&stream->input, 0, size);
}

void stream_write(stream *stream, void *base, size_t size)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), base, size);
}

void *stream_allocate(stream *stream, size_t size)
{
  size_t current_size = buffer_size(&stream->output);

  buffer_resize(&stream->output, current_size + size);
  return (char *) buffer_data(&stream->output) + current_size;
}

void stream_flush(stream *stream)
{
  (stream->ssl ? stream_ssl_write : stream_socket_write)(stream);
}
