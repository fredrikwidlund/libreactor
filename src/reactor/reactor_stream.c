#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_descriptor.h"
#include "reactor_stream.h"

static int reactor_stream_error(reactor_stream *stream)
{
  return reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_ERROR, NULL);
}

static int reactor_stream_hangup(reactor_stream *stream)
{
  return reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_CLOSE, NULL);
}

static void reactor_stream_input_save(reactor_stream *stream, void *data, size_t size)
{
  if (size)
    buffer_insert(&stream->read, buffer_size(&stream->read), data, size);
}

static int reactor_stream_input_exception(reactor_stream *stream, ssize_t n)
{
  if (n > 0)
    return REACTOR_OK;
  else if (n == 0)
    return reactor_stream_hangup(stream);
  else if (errno == EAGAIN)
    return REACTOR_OK;
  else
    return reactor_stream_error(stream);
}

static int reactor_stream_input_buffered(reactor_stream *stream)
{
  char block[REACTOR_STREAM_BLOCK_SIZE];
  ssize_t n, n_total;
  struct iovec iov;
  int e;

  n_total = 0;
  while (1)
    {
      n = read(reactor_descriptor_fd(&stream->descriptor), block, sizeof block);
      if (n <= 0)
        break;
      buffer_insert(&stream->read, buffer_size(&stream->read), block, n);
      n_total += n;
    }

  if (n_total > 0)
    {
      iov.iov_base = buffer_data(&stream->read);
      iov.iov_len = buffer_size(&stream->read);
      e = reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &iov);
      if (e == REACTOR_ABORT)
        return REACTOR_ABORT;
      buffer_erase(&stream->read, 0, buffer_size(&stream->read) - iov.iov_len);
    }

  return reactor_stream_input_exception(stream, n);
}

static int reactor_stream_input(reactor_stream *stream)
{
  char block[REACTOR_STREAM_BLOCK_SIZE];
  ssize_t n;
  struct iovec iov;
  int e;

  if (buffer_size(&stream->read))
    return reactor_stream_input_buffered(stream);

  n = read(reactor_descriptor_fd(&stream->descriptor), block, sizeof block);
  if (n == sizeof block)
    {
      reactor_stream_input_save(stream, block, sizeof block);
      return reactor_stream_input_buffered(stream);
    }

  if (n > 0)
    {
      iov.iov_base = block;
      iov.iov_len = n;
      e = reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &iov);
      if (e == REACTOR_ABORT)
        return REACTOR_ABORT;
      reactor_stream_input_save(stream, iov.iov_base, iov.iov_len);
    }

  return reactor_stream_input_exception(stream, n);
}

static int reactor_stream_output(reactor_stream *stream)
{
  int e;

  e = reactor_stream_flush(stream);
  if (e == REACTOR_ERROR)
    return reactor_stream_error(stream);
  if (!reactor_stream_blocked(stream))
    return reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_WRITE, NULL);
  return REACTOR_OK;
}

#ifndef GCOV_BUILD
static
#endif
int reactor_stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  int e;

  (void) type;
  switch (*(int *) data)
    {
    case EPOLLIN:
      return reactor_stream_input(stream);
    case EPOLLIN | EPOLLOUT:
      e = reactor_stream_input(stream);
      if (e == REACTOR_OK)
        e = reactor_stream_output(stream);
      return e;
    case EPOLLIN | EPOLLHUP:
    case EPOLLIN | EPOLLRDHUP:
    case EPOLLIN | EPOLLHUP | EPOLLRDHUP:
      e = reactor_stream_input(stream);
      if (e == REACTOR_OK)
        e = reactor_stream_hangup(stream);
      return e;
    case EPOLLOUT:
      return reactor_stream_output(stream);
    case EPOLLHUP:
    case EPOLLHUP | EPOLLRDHUP:
      return reactor_stream_hangup(stream);
    default:
      return reactor_stream_error(stream);
    }
}

int reactor_stream_open(reactor_stream *stream, reactor_user_callback *callback, void *state, int fd)
{
  reactor_user_construct(&stream->user, callback, state);
  buffer_construct(&stream->read);
  buffer_construct(&stream->write);
  stream->blocked = 0;
  return reactor_descriptor_open(&stream->descriptor, reactor_stream_event, stream, fd, EPOLLIN | EPOLLRDHUP | EPOLLET);
}

void reactor_stream_close(reactor_stream *stream)
{
  reactor_descriptor_close(&stream->descriptor);
  buffer_destruct(&stream->read);
  buffer_destruct(&stream->write);
}

int reactor_stream_blocked(reactor_stream *stream)
{
  return stream->blocked;
}

void reactor_stream_write(reactor_stream *stream, void *data, size_t size)
{
  buffer_insert(&stream->write, buffer_size(&stream->write), data, size);
}

void reactor_stream_write_string(reactor_stream *stream, char *string)
{
  reactor_stream_write(stream, string, strlen(string));
}

int reactor_stream_flush(reactor_stream *stream)
{
  ssize_t n, size;
  char *base;

  base = buffer_data(&stream->write);
  size = buffer_size(&stream->write);
  do
    {
      n = write(reactor_descriptor_fd(&stream->descriptor), base, size);
      if (n == -1)
        break;
      base += n;
      size -= n;
    }
  while (size);
  buffer_erase(&stream->write, 0, buffer_size(&stream->write) - size);

  if (size)
    {
      if (errno != EAGAIN)
        return REACTOR_ERROR;
      if (!reactor_stream_blocked(stream))
        {
          stream->blocked = 1;
          reactor_descriptor_set(&stream->descriptor, EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET);
        }
      return REACTOR_OK;
    }

  if (reactor_stream_blocked(stream))
    {
      stream->blocked = 0;
      reactor_descriptor_set(&stream->descriptor, EPOLLIN | EPOLLRDHUP | EPOLLET);
    }

  return REACTOR_OK;
}

buffer *reactor_stream_buffer(reactor_stream *stream)
{
  return &stream->write;
}
