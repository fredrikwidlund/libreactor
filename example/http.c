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

typedef struct app app;
struct app
{
  char         *host;
  char         *port;
  char         *resource;
  reactor_tcp   tcp;
  reactor_http  http;
  int           client;
};

typedef struct app_transaction app_transaction;
struct app_transaction
{
  app          *app;
  reactor_http  http;
  int           client;
};

static void http_event(void *state, int type, void *data)
{
  app_transaction *tx = state;
  reactor_http_response *response = data;

  switch (type)
    {
    case REACTOR_HTTP_EVENT_HANGUP:
    case REACTOR_HTTP_EVENT_ERROR:
      reactor_http_close(&tx->http);
      break;
    case REACTOR_HTTP_EVENT_CLOSE:
      free(tx);
      break;
    case REACTOR_HTTP_EVENT_RESPONSE:
      (void) fprintf(stdout, "%.*s", (int) response->size, (char *) response->data);
      reactor_http_close(&tx->http);
      break;
    case REACTOR_HTTP_EVENT_REQUEST:
      reactor_http_write_response(&tx->http, (reactor_http_response[]){{1, 200, "OK",
              1, (reactor_http_header[]){{"Content-Type", "text/plain"}}, "Hello, World\n", 13}});
      break;
    }
}

static void tcp_event(void *state, int type, void *data)
{
  app *app = state;
  app_transaction *tx;
  int s;

  switch (type)
    {
    case REACTOR_TCP_EVENT_ERROR:
      (void) fprintf(stderr, "[tcp] error\n");
      reactor_tcp_close(&app->tcp);
      break;
    case REACTOR_TCP_EVENT_CONNECT:
      s = *(int *) data;
      tx = malloc(sizeof *tx);
      tx->app = app;
      reactor_http_open(&tx->http, http_event, tx, s, 0);
      reactor_http_write_request(&tx->http, (reactor_http_request[]){{"GET", app->resource, 1,
              2, (reactor_http_header[]){{"Host", app->host}, {"Connection", "close"}}, NULL, 0}});
      reactor_http_flush(&tx->http);
      break;
    case REACTOR_TCP_EVENT_ACCEPT:
      s = *(int *) data;
      tx = malloc(sizeof *tx);
      tx->app = app;
      reactor_http_open(&tx->http, http_event, tx, s, REACTOR_HTTP_FLAG_SERVER);
      break;
    }
}

int main(int argc, char **argv)
{
  app app;

  if (argc != 5 ||
      (strcmp(argv[1], "client") == 0 &&
       strcmp(argv[1], "server") == 0))
    err(1, "usage: http [client|server] <host> <port> <resource>");

  app.host = argv[2];
  app.port = argv[3];
  app.resource = argv[4];
  app.client = strcmp(argv[1], "client") == 0;

  reactor_core_construct();
  reactor_tcp_open(&app.tcp, tcp_event, &app, app.host, app.port, app.client ? 0 : REACTOR_TCP_FLAG_SERVER);
  reactor_core_run();
  reactor_core_destruct();
}
