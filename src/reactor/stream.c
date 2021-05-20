#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "stream.h"

static core_status stream_next(core_event *event)
{
  stream *stream = event->state;

  stream->next = 0;
  return core_dispatch(&stream->user, STREAM_ERROR, 0);
}

static void stream_abort(stream *stream)
{
  stream_close(stream);
  stream->next = core_next(NULL, stream_next, stream);
}

static void stream_send(stream *stream)
{
  buffer *b = &stream->output;
  size_t offset = 0, size = buffer_size(b);
  ssize_t n;

  while (size - offset)
  {
    if (stream_is_socket(stream))
      n = send(stream->fd, (char *) buffer_data(b) + offset, size - offset, 0);
    else
      n = write(stream->fd, (char *) buffer_data(b) + offset, size - offset);
    if (n == -1)
      break;
    offset += n;
  }
  buffer_erase(b, 0, offset);
}

static size_t stream_receive(stream *stream)
{
  buffer *b = &stream->input;
  size_t offset = buffer_size(b), size = 0;
  ssize_t n;

  do
  {
    buffer_reserve(b, offset + size + STREAM_BLOCK_SIZE);
    if (stream_is_socket(stream))
      n = recv(stream->fd, (char *) buffer_data(b) + offset + size, STREAM_BLOCK_SIZE, 0);
    else
      n = read(stream->fd, (char *) buffer_data(b) + offset + size, STREAM_BLOCK_SIZE);
    size += n > 0 ? n : 0;
  } while (n == STREAM_BLOCK_SIZE);

  if (n == 0)
    stream->flags &= ~STREAM_OPEN;
  b->size += size;
  return size;
}

static core_status stream_callback(core_event *event)
{
  stream *stream = event->state;
  core_status e;
  size_t size;

  if (dynamic_unlikely(event->data == EPOLLOUT))
  {
    stream_flush(stream);
    return buffer_size(&stream->output) ? CORE_OK : core_dispatch(&stream->user, STREAM_FLUSH, 0);
  }

  if (dynamic_likely(event->data & EPOLLIN))
  {
    size = stream_receive(stream);
    if (dynamic_likely(size))
    {
      e = core_dispatch(&stream->user, STREAM_READ, 0);
      if (dynamic_unlikely(e != CORE_OK))
        return e;
    }
    if (dynamic_likely(stream_is_open(stream) && event->data == EPOLLIN))
      return CORE_OK;
  }

  return core_dispatch(&stream->user, STREAM_CLOSE, 0);
}

static void stream_events(stream *stream, int events)
{
  if (stream->events == events)
    ;
  else if (!stream->events)
    core_add(NULL, stream_callback, stream, stream->fd, events);
  else if (!events)
    core_delete(NULL, stream->fd);
  else
    core_modify(NULL, stream->fd, events);
  stream->events = events;
}

void stream_construct(stream *stream, core_callback *callback, void *state)
{
  *stream = (struct stream) {.user = {.callback = callback, .state = state}, .fd = -1};
  buffer_construct(&stream->input);
  buffer_construct(&stream->output);
}

void stream_destruct(stream *stream)
{
  stream_close(stream);
  if (stream->next)
  {
    core_cancel(NULL, stream->next);
    stream->next = 0;
  }
  buffer_destruct(&stream->input);
  buffer_destruct(&stream->output);
}

void stream_open(stream *stream, int fd)
{
  struct stat st;
  int e;

  e = fstat(fd, &st);
  if (e == -1 || stream_is_open(stream))
  {
    stream_abort(stream);
    return;
  }

  stream->fd = fd;
  stream->flags |= STREAM_OPEN | (S_ISSOCK(st.st_mode) ? STREAM_SOCKET : 0);
  stream_events(stream, EPOLLIN | EPOLLET);
}

void stream_close(stream *stream)
{
  if (stream->fd >= 0)
  {
    stream_events(stream, 0);
    (void) close(stream->fd);
    stream->fd = -1;
    stream->flags &= ~STREAM_OPEN;
  }
}

segment stream_read(stream *stream)
{
  return segment_data(buffer_data(&stream->input), buffer_size(&stream->input));
}

segment stream_read_line(stream *stream)
{
  segment input;
  char *delim;

  input = stream_read(stream);
  delim = memchr(input.base, '\n', input.size);
  return delim ? segment_data(input.base, delim - (char *) input.base + 1) : segment_empty();
}

void stream_write(stream *stream, segment output)
{
  buffer_insert(&stream->output, buffer_size(&stream->output), output.base, output.size);
}

segment stream_allocate(stream *stream, size_t size)
{
  buffer_resize(&stream->output, buffer_size(&stream->output) + size);
  return segment_data((char *) buffer_data(&stream->output) + buffer_size(&stream->output) - size, size);
}

void stream_consume(stream *stream, size_t size)
{
  buffer_erase(&stream->input, 0, size);
}

void stream_notify(stream *stream)
{
  if (!stream_is_open(stream))
    return;

  stream_events(stream, EPOLLIN | EPOLLOUT | EPOLLET);
}

void stream_flush(stream *stream)
{
  if (!stream_is_open(stream))
    return;

  stream_send(stream);
  stream_events(stream, buffer_size(&stream->output) ? EPOLLIN | EPOLLOUT | EPOLLET : EPOLLIN | EPOLLET);
}

int stream_is_open(stream *stream)
{
  return stream->flags & STREAM_OPEN;
}

int stream_is_socket(stream *stream)
{
  return stream->flags & STREAM_SOCKET;
}
