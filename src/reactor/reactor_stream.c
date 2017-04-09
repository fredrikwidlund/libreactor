#include <stdlib.h>
#include <stdint.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_stream.h"

static void reactor_stream_close_fd(reactor_stream *stream)
{
  reactor_core_fd_deregister(stream->fd);
  (void) close(stream->fd);
  stream->fd = -1;
}

static void reactor_stream_error(reactor_stream *stream)
{
  ((struct pollfd *) reactor_core_fd_poll(stream->fd))->events = 0;
  stream->state = REACTOR_STREAM_STATE_ERROR;
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_ERROR, NULL);
}

static void reactor_stream_event(void *state, int type, void *arg)
{
  reactor_stream *stream = state;
  short revents = ((struct pollfd *) arg)->revents;
  reactor_stream_data data;
  char buffer[REACTOR_STREAM_BLOCK_SIZE];
  ssize_t n;

  (void) type;
  if (reactor_likely(revents == POLLIN && !buffer_size(&stream->input)))
    {
      n = read(stream->fd, buffer, sizeof buffer);
      if (n <= 0)
        return;
      data = (reactor_stream_data) {.base = buffer, .size = n};
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &data);
      if (reactor_unlikely(data.size))
        buffer_insert(&stream->input, buffer_size(&stream->input), data.base, data.size);
    }
  else
    {
      reactor_stream_hold(stream);
      if (reactor_unlikely(revents & (POLLERR | POLLNVAL)))
        reactor_stream_error(stream);
      else
        {
          if (reactor_unlikely(revents & POLLOUT))
            reactor_stream_flush(stream);
          if (reactor_unlikely((revents & (POLLIN|POLLHUP)) == POLLHUP))
            reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_HANGUP, NULL);
          else if (reactor_likely(revents & POLLIN))
            {
              n = read(stream->fd, buffer, sizeof buffer);
              if (reactor_unlikely(n == 0))
                reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_HANGUP, NULL);
              else if (reactor_unlikely(n == -1))
                {
                  if (errno != EAGAIN)
                    reactor_stream_error(stream);
                }
              else
                {
                  if (reactor_likely(!buffer_size(&stream->input)))
                    {
                      data = (reactor_stream_data) {.base = buffer, .size = n};
                      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_READ, &data);
                      if (reactor_unlikely(data.size))
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
        }
      reactor_stream_release(stream);
    }
}

void reactor_stream_hold(reactor_stream *stream)
{
  stream->ref ++;
}

void reactor_stream_release(reactor_stream *stream)
{
  stream->ref --;
  if (reactor_unlikely(!stream->ref))
    {
      reactor_stream_close_fd(stream);
      buffer_destruct(&stream->input);
      buffer_destruct(&stream->output);
      stream->state = REACTOR_STREAM_STATE_CLOSED;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_CLOSE, NULL);
    }
}

void reactor_stream_open(reactor_stream *stream, reactor_user_callback *callback, void *state, int fd)
{
  stream->ref = 0;
  stream->state = REACTOR_STREAM_STATE_OPEN;
  reactor_user_construct(&stream->user, callback, state);
  stream->fd = fd;
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
  (void) fcntl(stream->fd, F_SETFL,O_NONBLOCK);
  reactor_core_fd_register(stream->fd, reactor_stream_event, stream, POLLIN);
  reactor_stream_hold(stream);
}

void reactor_stream_close(reactor_stream *stream)
{
  if (stream->state & REACTOR_STREAM_STATE_CLOSED)
    return;

  reactor_stream_hold(stream);
  if (stream->state & (REACTOR_STREAM_STATE_OPEN | REACTOR_STREAM_STATE_CLOSING))
    {
      stream->state = REACTOR_STREAM_STATE_CLOSING;
      if (buffer_size(&stream->output))
        reactor_stream_flush(stream);
      if (stream->state & REACTOR_STREAM_STATE_CLOSING && buffer_size(&stream->output) == 0)
        reactor_stream_release(stream);
    }

  if (stream->state & REACTOR_STREAM_STATE_ERROR)
    reactor_stream_release(stream);
  reactor_stream_release(stream);
}

void reactor_stream_write(reactor_stream *stream, void *data, size_t size)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), (void *) data, size);
}

void reactor_stream_write_unsigned(reactor_stream *stream, uint32_t n)
{
  static const uint32_t pow10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  static const char digits[200] =
    "0001020304050607080910111213141516171819202122232425262728293031323334353637383940414243444546474849"
    "5051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
  uint32_t t, size, x;
  buffer *b;
  char *base;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  size = t - (n < pow10[t]) + 1;
  b = &stream->output;
  buffer_reserve(b, buffer_size(b) + size);
  b->size += size;
  base = (char *) buffer_data(b) + buffer_size(b);

  while (n >= 100)
    {
      x = (n % 100) << 1;
      n /= 100;
      *--base = digits[x + 1];
      *--base = digits[x];
    }
  if (n >= 10)
    {
      x = n << 1;
      *--base = digits[x + 1];
      *--base = digits[x];
    }
  else
    *--base = n + '0';
}

void reactor_stream_flush(reactor_stream *stream)
{
  char *base;
  ssize_t size, n;
  int saved_state;

  errno = 0;
  base = buffer_data(&stream->output);
  size = buffer_size(&stream->output);
  if (!size)
    return;

  do
    {
      n = write(stream->fd, base, size);
      if (reactor_unlikely(n == -1))
        break;
      base += n;
      size -= n;
    }
  while (size);

  buffer_erase(&stream->output, 0, buffer_size(&stream->output) - size);
  if (reactor_unlikely(buffer_size(&stream->output) == 0))
    {
      if (stream->state == REACTOR_STREAM_STATE_OPEN)
        {
          ((struct pollfd *) reactor_core_fd_poll(stream->fd))->events &= ~POLLOUT;
          reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_WRITE, NULL);
        }
      else
        reactor_stream_close(stream);
      return;
    }

  if (reactor_unlikely(errno == EAGAIN))
    {
      ((struct pollfd *) reactor_core_fd_poll(stream->fd))->events |= POLLOUT;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_EVENT_BLOCKED, NULL);
      return;
    }

  reactor_stream_hold(stream);
  saved_state = stream->state;
  reactor_stream_error(stream);
  if (reactor_unlikely(saved_state == REACTOR_STREAM_STATE_CLOSING))
    reactor_stream_close(stream);
  reactor_stream_release(stream);
}

void reactor_stream_write_notify(reactor_stream *stream)
{
  ((struct pollfd *) reactor_core_fd_poll(stream->fd))->events |= POLLOUT;
}

void *reactor_stream_data_base(reactor_stream_data *data)
{
  return data->base;
}

size_t reactor_stream_data_size(reactor_stream_data *data)
{
  return data->size;
}

void reactor_stream_data_consume(reactor_stream_data *data, size_t size)
{
  data->base = (char *) data->base + size;
  data->size -= size;
}
