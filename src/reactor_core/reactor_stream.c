#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"

void reactor_stream_init(reactor_stream *stream, reactor_user_callback *callback, void *state)
{
  *stream = (reactor_stream) {.state = REACTOR_STREAM_CLOSED};
  reactor_user_init(&stream->user, callback, state);
  reactor_desc_init(&stream->desc, reactor_stream_event, stream);
  buffer_init(&stream->input);
  buffer_init(&stream->output);
}

int reactor_stream_open(reactor_stream *stream, int fd)
{
  int e;

  if (stream->state != REACTOR_STREAM_CLOSED)
    return -1;

  e = reactor_desc_open(&stream->desc, fd);
  if (e == -1)
    return -1;

  stream->state = REACTOR_STREAM_OPEN;
  return 0;
}

void reactor_stream_close(reactor_stream *stream)
{
  if (stream->state == REACTOR_STREAM_CLOSED)
    return;

  shutdown(reactor_desc_fd(&stream->desc), SHUT_RDWR);
  reactor_desc_close(&stream->desc);
  buffer_clear(&stream->input);
  buffer_clear(&stream->output);
  stream->state = REACTOR_STREAM_CLOSED;
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_CLOSE, NULL);
}

void reactor_stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch(type)
    {
    case REACTOR_DESC_ERROR:
      reactor_stream_error(stream);
      break;
    case REACTOR_DESC_READ:
      reactor_stream_read(stream);
      if (reactor_core_current() >= 0 && (stream->flags & REACTOR_STREAM_FLAGS_BLOCKED) == 0)
        reactor_stream_flush(stream);
      break;
    case REACTOR_DESC_WRITE:
      stream->flags &= ~REACTOR_STREAM_FLAGS_BLOCKED;
      reactor_stream_flush(stream);
      if (reactor_core_current() >= 0 && (stream->flags & REACTOR_STREAM_FLAGS_BLOCKED) == 0)
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_WRITE_AVAILABLE, NULL);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_stream_close(stream);
      break;
    }
}

void reactor_stream_error(reactor_stream *stream)
{
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_ERROR, NULL);
  if (reactor_core_current() >= 0)
    reactor_stream_close(stream);
}

void reactor_stream_read(reactor_stream *stream)
{
  char buffer[REACTOR_STREAM_BLOCK_SIZE];
  ssize_t n;

  n = recv(reactor_desc_fd(&stream->desc), buffer, sizeof buffer, MSG_DONTWAIT);
  if (n == -1 && errno != EAGAIN)
    reactor_stream_error(stream);
  else if (n == 0)
    reactor_stream_close(stream);
  else if (n > 0)
    reactor_user_dispatch(&stream->user, REACTOR_STREAM_READ, (reactor_stream_data[]){{.base = buffer, .size = n}});
}

void reactor_stream_write_direct(reactor_stream *stream, void *base, size_t size)
{
  ssize_t n;

  if (buffer_size(&stream->output))
    reactor_stream_write(stream, base, size);
  else while (size)
    {
      n = send(reactor_desc_fd(&stream->desc), base, size, MSG_DONTWAIT);
      if (n == -1)
        {
          if (errno != EAGAIN)
            {
              reactor_stream_error(stream);
              return;
            }

          stream->flags |= REACTOR_STREAM_FLAGS_BLOCKED;
          reactor_desc_events(&stream->desc, REACTOR_DESC_READ | REACTOR_DESC_WRITE);
          reactor_stream_write(stream, base, size);
          return;
        }

      base = (char *) base +  n;
      size -= n;
    }
}

void reactor_stream_write(reactor_stream *stream, void *base, size_t size)
{
  int e;

  e = buffer_insert(&stream->output, buffer_size(&stream->output), base, size);
  if (e == -1)
    reactor_stream_error(stream);
}

void reactor_stream_flush(reactor_stream *stream)
{
  ssize_t n;

  while (stream->state > REACTOR_STREAM_CLOSED && buffer_size(&stream->output))
    {
      n = send(reactor_desc_fd(&stream->desc), buffer_data(&stream->output), buffer_size(&stream->output), MSG_DONTWAIT);
      if (n == -1)
        {
          if (errno != EAGAIN)
            {
              reactor_stream_error(stream);
              return;
            }

          stream->flags |= REACTOR_STREAM_FLAGS_BLOCKED;
          reactor_desc_events(&stream->desc, REACTOR_DESC_READ | REACTOR_DESC_WRITE);
          return;
        }

      buffer_erase(&stream->output, 0, n);
    }
}
