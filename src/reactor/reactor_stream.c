#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_stream.h"

static void reactor_stream_error(reactor_stream *stream)
{
  stream->state = REACTOR_STREAM_STATE_ERROR;
  ((struct pollfd *) reactor_core_poll(stream->fd))->events = 0;
  buffer_clear(&stream->output);
  buffer_clear(&stream->input);
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_ERROR, NULL);
}

static void reactor_stream_event(void *state, __attribute__ ((unused)) int type, void *event_data)
{
  reactor_stream *stream = state;
  short revents = ((struct pollfd *) event_data)->revents;
  char buffer[REACTOR_STREAM_BLOCK_SIZE];
  reactor_stream_data data;
  ssize_t n;

  if (revents & (POLLERR | POLLNVAL))
    {
      reactor_stream_error(stream);
      return;
    }

  reactor_stream_hold(stream);
  if (revents & POLLOUT)
    reactor_stream_flush(stream);

  if (revents & (POLLIN|POLLHUP) && stream->state == REACTOR_STREAM_STATE_OPEN)
    {
      if (revents & POLLIN)
        n = read(stream->fd, buffer, sizeof buffer);
      if ((revents & POLLIN) == 0 || n == 0)
        reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_HANGUP, NULL);
      else if (n == -1)
        {
          if (errno != EAGAIN)
            reactor_stream_error(stream);
        }
      else
        {
          if (!buffer_size(&stream->input))
            {
              data = (reactor_stream_data) {.base = buffer, .size = n};
              reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &data);
              if (data.size)
                buffer_insert(&stream->input, buffer_size(&stream->input), data.base, data.size);
            }
          else
            {
              buffer_insert(&stream->input, buffer_size(&stream->input), buffer, n);
              data = (reactor_stream_data) {.base = buffer_data(&stream->input), .size = buffer_size(&stream->input)};
              reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &data);
              buffer_erase(&stream->input, 0, buffer_size(&stream->input) - data.size);
            }
        }
    }
  reactor_stream_release(stream);
}

void reactor_stream_hold(reactor_stream *stream)
{
  stream->ref ++;
}

void reactor_stream_release(reactor_stream *stream)
{
  stream->ref --;
  if (!stream->ref)
    {
      buffer_destruct(&stream->input);
      buffer_destruct(&stream->output);
      reactor_core_deregister(stream->fd);
      (void) close(stream->fd);
      stream->fd = -1;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_CLOSE, NULL);
    }
}

void reactor_stream_open(reactor_stream *stream, reactor_user_callback *callback, void *state, int fd)
{
  stream->state = REACTOR_STREAM_STATE_OPEN;
  reactor_user_construct(&stream->user, callback, state);
  stream->ref = 0;
  stream->fd = fd;
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
  (void) fcntl(stream->fd, F_SETFL,O_NONBLOCK);
  reactor_core_register(stream->fd, reactor_stream_event, stream, POLLIN);
  reactor_stream_hold(stream);
}

void reactor_stream_close(reactor_stream *stream)
{
  switch(stream->state)
    {
    case REACTOR_STREAM_STATE_LINGER:
    case REACTOR_STREAM_STATE_OPEN:
      if (buffer_size(&stream->output))
        {
          //((struct pollfd *) reactor_core_poll(stream->fd))->events = POLLOUT;
          stream->state = REACTOR_STREAM_STATE_LINGER;
          reactor_stream_flush(stream);
          break;
        }
      /* fall through */
    case REACTOR_STREAM_STATE_ERROR:
      stream->state = REACTOR_STREAM_STATE_CLOSED;
      reactor_stream_release(stream);
      break;
    }
}

void reactor_stream_write(reactor_stream *stream, void *data, size_t size)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), data, size);
  ((struct pollfd *) reactor_core_poll(stream->fd))->events |= POLLOUT;
}

void reactor_stream_flush(reactor_stream *stream)
{
  char *base;
  ssize_t size, n;
  int saved_state;

  errno = 0;
  base = buffer_data(&stream->output);
  size = buffer_size(&stream->output);
  while (size)
    {
      n = write(stream->fd, base, size);
      if (n == -1)
        break;
      base += n;
      size -= n;
    }
  buffer_erase(&stream->output, 0, buffer_size(&stream->output) - size);
  if (buffer_size(&stream->output) == 0)
    {
      if (stream->state == REACTOR_STREAM_STATE_OPEN)
        {
          ((struct pollfd *) reactor_core_poll(stream->fd))->events &= ~POLLOUT;
          reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_WRITE, NULL);
        }
      else
        reactor_stream_close(stream);
      return;
    }

  if (errno == EAGAIN)
    {
      ((struct pollfd *) reactor_core_poll(stream->fd))->events |= POLLOUT;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_BLOCKED, NULL);
      return;
    }

  reactor_stream_hold(stream);
  saved_state = stream->state;
  reactor_stream_error(stream);
  if (saved_state == REACTOR_STREAM_STATE_LINGER)
    reactor_stream_close(stream);
  reactor_stream_release(stream);
}

void reactor_stream_write_notify(reactor_stream *stream)
{
  ((struct pollfd *) reactor_core_poll(stream->fd))->events |= POLLOUT;
}

void reactor_stream_data_consume(reactor_stream_data *data, size_t size)
{
  data->size -= size;
}
