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

#include "reactor.h"

void client_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  reactor_stream_data *read = data;
  static char reply[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Length: 4\r\n"
    "Content-Type: text/plain\r\n"
    "\r\n"
    "test";

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_READ:
      reactor_stream_consume(read, read->size);
      reactor_stream_write(stream, reply, sizeof reply - 1);
      break;
    case REACTOR_STREAM_ERROR:
    case REACTOR_STREAM_SHUTDOWN:
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_CLOSE:
      free(stream);
      break;
    }
}

void event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  reactor_stream *stream;

  (void) state;
  switch (type)
    {
    case REACTOR_TCP_ACCEPT:
      stream = malloc(sizeof *stream);
      reactor_stream_init(stream, client_event, stream);
      reactor_stream_open(stream, *(int *) data);
      break;
    case REACTOR_TCP_ERROR:
    case REACTOR_TCP_SHUTDOWN:
      reactor_tcp_close(tcp);
      break;
    }
}

int main()
{
  reactor_tcp tcp;

  reactor_core_open();
  reactor_tcp_init(&tcp, event, &tcp);
  reactor_tcp_open(&tcp, NULL, "80", REACTOR_TCP_SERVER);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
