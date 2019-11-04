#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_stream_error(reactor_stream *stream, char *description)
{
  return reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_ERROR, (uintptr_t) description);
}

static void reactor_stream_write_set(reactor_stream *stream)
{
  if (~stream->flags & REACTOR_STREAM_FLAG_WRITE)
    {
      stream->flags |= REACTOR_STREAM_FLAG_WRITE;
      reactor_fd_events(&stream->fd, EPOLLIN | EPOLLOUT | EPOLLET);
    }
}

static void reactor_stream_write_clear(reactor_stream *stream)
{
  if (stream->flags & REACTOR_STREAM_FLAG_WRITE)
    {
      stream->flags &= ~REACTOR_STREAM_FLAG_WRITE;
      reactor_fd_events(&stream->fd, EPOLLIN | EPOLLET);
    }
}

static reactor_status reactor_stream_input(reactor_stream *stream)
{
  buffer *b;
  ssize_t n, n_read;
  int e;

  b = &stream->input;
  n_read = 0;
  while (1)
    {
      buffer_reserve(b, buffer_size(b) + REACTOR_STREAM_BLOCK_SIZE);
      n = read(reactor_fd_fileno(&stream->fd), (char *) buffer_data(b) + buffer_size(b), buffer_capacity(b) - buffer_size(b));
      if (n > 0)
        {
          n_read += n;
          b->size += n;
          if (buffer_size(b) < buffer_capacity(b))
            break;
        }
      else
        {
          if (n == -1 && errno != EAGAIN)
            return reactor_stream_error(stream, "read error");
          break;
        }
    }

  if (n_read)
    {
      e = reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_DATA, 0);
      if (e != REACTOR_OK)
        return e;
    }

  if (n == 0)
    return reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_CLOSE, 0);

  return REACTOR_OK;
}

static reactor_status reactor_stream_handler(reactor_event *event)
{
  reactor_stream *stream = event->state;
  int e;

  if (event->data & EPOLLIN)
    {
      e = reactor_stream_input(stream);
      if (e != REACTOR_OK)
        return e;
    }

  if (event->data & EPOLLOUT)
    {
      e = reactor_stream_flush(stream);
      if (e != REACTOR_OK)
        return reactor_stream_error(stream, "write error");

      if (stream->flags & REACTOR_STREAM_FLAG_SHUTDOWN &&
          !buffer_size(&stream->output))
        (void) shutdown(reactor_fd_fileno(&stream->fd), SHUT_RDWR);
    }

  return REACTOR_OK;
}

void reactor_stream_construct(reactor_stream *stream, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&stream->user, callback, state);
  reactor_fd_construct(&stream->fd, reactor_stream_handler, stream);
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
}

void reactor_stream_user(reactor_stream *stream, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&stream->user, callback, state);
}

void reactor_stream_destruct(reactor_stream *stream)
{
  reactor_fd_destruct(&stream->fd);
  buffer_destruct(&stream->input);
  buffer_destruct(&stream->output);
}

void reactor_stream_reset(reactor_stream *stream)
{
  reactor_fd_close(&stream->fd);
  buffer_clear(&stream->input);
  buffer_clear(&stream->output);
}

void reactor_stream_open(reactor_stream *stream, int fd)
{
  stream->flags = 0;
  reactor_fd_open(&stream->fd, fd, EPOLLIN | EPOLLET);
}

void *reactor_stream_data(reactor_stream *stream)
{
  return buffer_data(&stream->input);
}

size_t reactor_stream_size(reactor_stream *stream)
{
  return buffer_size(&stream->input);
}

void reactor_stream_consume(reactor_stream *stream, size_t size)
{
  buffer_erase(&stream->input, 0, size);
}

void *reactor_stream_segment(reactor_stream *stream, size_t size)
{
  size_t current_size = buffer_size(&stream->output);

  buffer_reserve(&stream->output, current_size + size);
  stream->output.size += size;
  return (char *) buffer_data(&stream->output) + current_size;
}

void reactor_stream_write(reactor_stream *stream, void *data, size_t size)
{
  memcpy(reactor_stream_segment(stream, size), data, size);
}

reactor_status reactor_stream_flush(reactor_stream *stream)
{
  buffer *b;
  ssize_t n;
  char *data;
  size_t size;

  b = &stream->output;
  data = buffer_data(b);
  size = buffer_size(b);
  while (size)
    {
      n = write(reactor_fd_fileno(&stream->fd), data, size);
      if (n == -1 && errno == EAGAIN)
        break;
      if (n == -1)
        return REACTOR_ERROR;
      data += n;
      size -= n;
    }

  buffer_erase(b, 0, buffer_size(b) - size);

  if (buffer_size(b))
    reactor_stream_write_set(stream);
  else
    reactor_stream_write_clear(stream);

  return REACTOR_OK;
}

void reactor_stream_shutdown(reactor_stream *stream)
{
  if (buffer_size(&stream->output))
    stream->flags |= REACTOR_STREAM_FLAG_SHUTDOWN;
  else
    (void) shutdown(reactor_fd_fileno(&stream->fd), SHUT_RDWR);
}
