#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

typedef struct client client;
typedef struct server server;
typedef struct transaction transaction;

struct client
{
  reactor_http   http;
  server        *server;
};

struct server
{
  reactor_tcp    tcp;
  reactor_timer  timer;
};

struct transaction
{
  client        *client;
  reactor_tcp    tcp;
  reactor_http   http;
};

static char date[] = "Thu, 01 Jan 1970 00:00:00 GMT";

int timer_event(void *state, int type, void *data)
{
  (void) state;
  (void) data;
  if (type != REACTOR_TIMER_EVENT_CALL)
    err(1, "timer");

  reactor_http_date(date);
  return REACTOR_OK;
}

int http_event(void *state, int type, void *data)
{
  transaction *tx = state;
  reactor_http_response *response;

  switch (type)
    {
    case REACTOR_HTTP_EVENT_RESPONSE:
      response = data;
      reactor_http_send_response(&tx->client->http, response);
      reactor_http_flush(&tx->client->http);
      reactor_http_close(&tx->http);
      reactor_tcp_close(&tx->tcp);
      free(tx);
      break;
    default:
      err(1, "http");
    }
  return REACTOR_OK;
}

int tcp_event(void *state, int type, void *data)
{
  transaction *tx = state;
  char host[4096];
  int e;

  reactor_tcp_close(&tx->tcp);
  if (type != REACTOR_TCP_EVENT_CONNECT)
    err(1, "tcp");

  (void) snprintf(host, sizeof host, "%s", "127.0.0.1");
  e = reactor_http_open(&tx->http, http_event, tx, *(int *) data, 0);
  if (e != REACTOR_OK)
    err(1, "reactor_http_open");
  reactor_http_send_request(&tx->http, (reactor_http_request[]){{
        .method = {"GET", 3},
        .path = {"/", 1},
        .headers = 1,
        .header[0] = {{"Host", 4}, {host, strlen(host)}},
        .version = 1,
      }});
  return reactor_http_flush(&tx->http);
}

int client_event(void *state, int type, void *data)
{
  client *client = state;
  transaction *tx;
  reactor_http_request *request = data;
  int e;

  if (reactor_unlikely(type != REACTOR_HTTP_EVENT_REQUEST))
    {
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

  if (request->method.iov_len == 4 && strncmp(request->method.iov_base, "QUIT", 4) == 0)
    {
      reactor_timer_close(&client->server->timer);
      reactor_tcp_close(&client->server->tcp);
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

  tx = malloc(sizeof *tx);
  if (!tx)
    abort();
  tx->client = client;

  e = reactor_tcp_open(&tx->tcp, tcp_event, tx, "127.0.0.1", "80", 0);
  if (e != REACTOR_OK)
    {
      free(tx);
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

  return REACTOR_OK;
}

int server_event(void *state, int type, void *data)
{
  server *server = state;
  client *client;

  if (type != REACTOR_TCP_EVENT_ACCEPT)
    err(1, "tcp");

  client = malloc(sizeof *client);
  if (!client)
    abort();
  client->server = server;
  reactor_http_open(&client->http, client_event, client, *(int *) data,
                    REACTOR_HTTP_FLAG_SERVER);
  return REACTOR_OK;
}

int main(int argc, char **argv)
{
  server server;
  int e;

  if (argc != 3)
    err(1, "usage");

  reactor_core_construct();
  reactor_http_date(date);
  e = reactor_timer_open(&server.timer, timer_event, &server, 1000000000, 1000000000);
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_timer_open");

  e = reactor_tcp_open(&server.tcp, server_event, &server, argv[1], argv[2], REACTOR_TCP_FLAG_SERVER);
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_tcp_open");

  e = reactor_core_run();
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_core_run");

  reactor_core_destruct();
}
