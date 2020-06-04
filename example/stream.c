#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>

#include <dynamic.h>
#include <reactor.h>

void process(stream *stream)
{
  void *base;
  size_t size;

  while (1)
    {
      stream_readline(stream, &base, &size);
      if (!size)
        return;
      (void) printf("%.*s", (int) size, (char *) base);
      stream_consume(stream, size);
    }
}

static core_status callback(core_event *event)
{
  switch (event->type)
    {
    case STREAM_READ:
      process(event->state);
      return CORE_OK;
    default:
      process(event->state);
      stream_destruct(event->state);
      return CORE_ABORT;
    }
}

int main()
{
  stream stream;

  (void) fcntl(0, F_SETFL, O_NONBLOCK);
  core_construct(NULL);
  stream_construct(&stream, callback, &stream);
  stream_open(&stream, 0);
  core_loop(NULL);
  core_destruct(NULL);
  fflush(stdout);
}
