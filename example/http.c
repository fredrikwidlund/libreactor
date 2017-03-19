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

void http_event(void *state, int type, void *data)
{
  reactor_http *http = state;

  (void) http;
  (void) data;
  switch (type)
    {
    }
}

void tcp_event(void *state, int type, void *data)
{
  reactor_tcp *tcp = state;
  reactor_http *http;
  int s;

  (void) tcp;
  switch (type)
    {
    case REACTOR_TCP_EVENT_ERROR:
      (void) fprintf(stderr, "error\n");
      reactor_tcp_close(tcp);
      break;
    case REACTOR_TCP_EVENT_CONNECT:
      s = *(int *) data;
      http = malloc(sizeof *http);
      reactor_http_open(http, http_event, http, s, 0);
      reactor_http_write_request(http, (reactor_http_request[]){{"GET", "/", 1,
              (reactor_http_header[]){{"Host", "test"}, {"Connection", "close"}, {0}}, NULL, 0}});
      reactor_http_flush(http);
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_tcp tcp;

  if (argc != 5 ||
      (strcmp(argv[1], "client") == 0 &&
       strcmp(argv[1], "server") == 0))
    err(1, "usage: http [client|server] <host> <port> <resource>");

  reactor_core_construct();
  reactor_tcp_open(&tcp, tcp_event, &tcp, argv[2], argv[3],
                   strcmp(argv[1], "server") == 0 ? REACTOR_TCP_FLAG_SERVER : 0);
  reactor_core_run();
  reactor_core_destruct();
}
