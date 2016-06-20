#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"

static inline void reactor_stream_close_final(reactor_stream *stream)
{
  if (stream->state == REACTOR_STREAM_CLOSE_WAIT &&
      stream->desc.state == REACTOR_DESC_CLOSED &&
      stream->ref == 0)
    {
      buffer_clear(&stream->input);
      buffer_clear(&stream->output);
      stream->state = REACTOR_STREAM_CLOSED;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_CLOSE, NULL);
    }
}

static inline void reactor_stream_hold(reactor_stream *stream)
{
  stream->ref ++;
}

static inline void reactor_stream_release(reactor_stream *stream)
{
  stream->ref --;
  reactor_stream_close_final(stream);
}

static size_t reactor_stream_desc_write(reactor_stream *stream, void *data, size_t size)
{
  size_t i;
  ssize_t n;

  for (i = 0; i < size; i += n)
    {
      n = reactor_desc_write(&stream->desc, (char *) data + i, size - i);
      if (n == -1)
        {
          stream->flags |= REACTOR_STREAM_FLAGS_BLOCKED;
          reactor_desc_set(&stream->desc, REACTOR_DESC_FLAGS_WRITE);
          break;
        }
    }
  return i;
}

void reactor_stream_init(reactor_stream *stream, reactor_user_callback *callback, void *state)
{
  *stream = (reactor_stream) {.state = REACTOR_STREAM_CLOSED};
  reactor_user_init(&stream->user, callback, state);
  reactor_desc_init(&stream->desc, reactor_stream_event, stream);
  buffer_init(&stream->input);
  buffer_init(&stream->output);
}

void reactor_stream_open(reactor_stream *stream, int fd)
{
  if (stream->state != REACTOR_STREAM_CLOSED)
    {
      (void) close(fd);
      reactor_stream_error(stream);
      return;
    }

  stream->state = REACTOR_STREAM_OPEN;
  reactor_desc_open(&stream->desc, fd, REACTOR_DESC_FLAGS_READ);
}

void reactor_stream_error(reactor_stream *stream)
{
  stream->state = REACTOR_STREAM_INVALID;
  reactor_user_dispatch(&stream->user, REACTOR_STREAM_ERROR, NULL);
}

void reactor_stream_shutdown(reactor_stream *stream)
{
  if (stream->state != REACTOR_STREAM_OPEN &&
      stream->state != REACTOR_STREAM_INVALID)
    return;

  if (stream->state == REACTOR_STREAM_OPEN && buffer_size(&stream->output))
    {
      stream->state = REACTOR_STREAM_LINGER;
      reactor_desc_clear(&stream->desc, REACTOR_DESC_FLAGS_READ);
      return;
    }

  reactor_stream_close(stream);
}

void reactor_stream_close(reactor_stream *stream)
{
  if (stream->state == REACTOR_STREAM_CLOSED)
    return;
  reactor_desc_close(&stream->desc);
  stream->state = REACTOR_STREAM_CLOSE_WAIT;
  reactor_stream_close_final(stream);
}

void reactor_stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  reactor_stream_hold(stream);
  switch(type)
    {
    case REACTOR_DESC_ERROR:
      reactor_stream_error(stream);
      break;
    case REACTOR_DESC_READ:
      if (stream->state != REACTOR_STREAM_OPEN)
        break;
      reactor_stream_read(stream);
      if ((stream->state == REACTOR_STREAM_OPEN ||
           stream->state == REACTOR_STREAM_LINGER ) &&
          (stream->flags & REACTOR_STREAM_FLAGS_BLOCKED) == 0)
        reactor_stream_flush(stream);
      break;
    case REACTOR_DESC_WRITE:
      stream->flags &= ~REACTOR_STREAM_FLAGS_BLOCKED;
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_WRITE_AVAILABLE, NULL);
      if (stream->state == REACTOR_STREAM_OPEN &&
          (stream->flags & REACTOR_STREAM_FLAGS_BLOCKED) == 0)
        reactor_stream_flush(stream);
      break;
    case REACTOR_DESC_SHUTDOWN:
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_SHUTDOWN, NULL);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_stream_close_final(stream);
      break;
    }
  reactor_stream_release(stream);
}

void reactor_stream_read(reactor_stream *stream)
{
  char buffer[REACTOR_STREAM_BLOCK_SIZE];
  reactor_stream_data data;
  ssize_t n;

  n = reactor_desc_read(&stream->desc, buffer, sizeof buffer);
  if (n == -1 && errno == EAGAIN)
    return;

  if (n == -1)
    {
      reactor_stream_error(stream);
      return;
    }

  if (n == 0)
    {
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_SHUTDOWN, NULL);
      return;
    }

  reactor_stream_hold(stream);
  if (buffer_size(&stream->input) == 0)
    {
      data = (reactor_stream_data) {.base = buffer, .size = n};
      reactor_user_dispatch(&stream->user, REACTOR_STREAM_READ, &data);
      if (stream->state == REACTOR_STREAM_OPEN && data.size)
        buffer_insert(&stream->input, buffer_size(&stream->input), data.base, data.size);
    }
 else
   {
     buffer_insert(&stream->input, buffer_size(&stream->input), buffer, n);
     data = (reactor_stream_data) {.base = buffer_data(&stream->input), .size = buffer_size(&stream->input)};
     reactor_user_dispatch(&stream->user, REACTOR_STREAM_READ, &data);
     buffer_erase(&stream->input, 0, buffer_size(&stream->input) - data.size);
   }
  reactor_stream_release(stream);
}

void reactor_stream_write(reactor_stream *stream, void *base, size_t size)
{
  int e;

  e = buffer_insert(&stream->output, buffer_size(&stream->output), base, size);
  if (e == -1)
    reactor_stream_error(stream);
}

void reactor_stream_write_direct(reactor_stream *stream, void *base, size_t size)
{
  size_t n;

  if (stream->state != REACTOR_STREAM_OPEN && stream->state != REACTOR_STREAM_LINGER)
    return;

  if (buffer_size(&stream->output))
    {
      reactor_stream_write(stream, base, size);
      return;
    }

  n = reactor_stream_desc_write(stream, base, size);
  if (n < size)
    {
      if (errno != EAGAIN)
        reactor_stream_error(stream);
      else
        reactor_stream_write(stream, (char *) base + n, size - n);
    }
}

void reactor_stream_write_string(reactor_stream *stream, char *string)
{
  reactor_stream_write(stream, string, strlen(string));
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
  int e;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  size = t - (n < pow10[t]) + 1;

  b = &stream->output;
  e = buffer_reserve(b, buffer_size(b) + size);
  if (e == -1)
    {
      reactor_stream_error(stream);
      return;
    }
  b->size += size;
  base = buffer_data(b) + buffer_size(b);

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
  ssize_t n;

  if (stream->state != REACTOR_STREAM_OPEN && stream->state != REACTOR_STREAM_LINGER)
    return;

  if (!buffer_size(&stream->output))
    return;

  n = reactor_stream_desc_write(stream, buffer_data(&stream->output), buffer_size(&stream->output));
  if (n == -1 && errno != EAGAIN)
    {
      reactor_stream_error(stream);
      return;
    }
  buffer_erase(&stream->output, 0, n);
  if (buffer_size(&stream->output))
    reactor_desc_set(&stream->desc, REACTOR_DESC_FLAGS_WRITE);
  if (stream->state == REACTOR_STREAM_LINGER && buffer_size(&stream->output) == 0)
    reactor_stream_close(stream);

}

void reactor_stream_consume(reactor_stream_data *data, size_t size)
{
  data->base += size;
  data->size -= size;
}
