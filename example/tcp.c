#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

void stream_event(void *state, int type, void *data)
{
  reactor_stream *stream = state;
  reactor_stream_data *read = data;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      (void) printf("[read] %.*s\n", (int) read->size, (char *) read->base);
      if (memmem(read->base, read->size, "quit", 4))
        reactor_stream_close(stream);
      reactor_stream_data_consume(read, read->size);
      break;
    case REACTOR_STREAM_EVENT_ERROR:
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      (void) printf("[close]\n");
      free(stream);
      break;
    }
}

void tcp_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  reactor_stream *stream;
  int s;

  (void) tcp;
  switch (type)
    {
    case REACTOR_TCP_EVENT_ERROR:
      (void) fprintf(stderr, "error\n");
      reactor_tcp_close(tcp);
      break;
    case REACTOR_TCP_EVENT_ACCEPT:
      s = *(int *) data;
      (void) printf("[accept %d]\n", s);
      stream = malloc(sizeof *stream);
      reactor_stream_open(stream, stream_event, stream, s);
      break;
    case REACTOR_TCP_EVENT_CONNECT:
      s = *(int *) data;
      (void) printf("[connect %d]\n", s);
      stream = malloc(sizeof *stream);
      reactor_stream_open(stream, stream_event, stream, s);
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_tcp tcp;

  if (argc != 4 ||
      (strcmp(argv[1], "client") == 0 &&
       strcmp(argv[1], "server") == 0))
    err(1, "usage: tcp [client|server] <host> <port>");

  reactor_core_construct();
  reactor_tcp_open(&tcp, tcp_event, &tcp, argv[2], argv[3],
                   strcmp(argv[1], "server") == 0 ? REACTOR_TCP_FLAG_SERVER : 0);
  reactor_core_run();
  reactor_core_destruct();
}
