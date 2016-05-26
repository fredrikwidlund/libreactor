#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <assert.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_core.h"

static char reply[] =
  "HTTP/1.0 200 OK\r\n"
  "Content-Length: 4\r\n"
  "Content-Type: plain/html\r\n"
  "Connection: keep-alive\r\n"
  "\r\n"
  "test";

void client_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_ERROR:
      break;
    case REACTOR_STREAM_READ:
      reactor_stream_write(stream, reply, sizeof reply - 1);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    }
}

void event(void *state, int type, void *data)
{
  reactor_stream *stream;
  int e;

  (void) state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      stream = malloc(sizeof *stream);
      reactor_stream_init(stream, client_event, stream);
      e = reactor_stream_open(stream, *(int *) data);
      if (e == -1)
        err(1, "reactor_stream_open");
      break;
    case REACTOR_TCP_ERROR:
      err(1, "event");
      break;
    }
}

int main()
{
  reactor_tcp tcp;

  reactor_core_open();
  reactor_tcp_init(&tcp, event, &tcp);
  reactor_tcp_listen(&tcp, NULL, "80");
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
