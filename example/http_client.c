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

struct client
{
  char         *node;
  char         *service;
  char         *path;
  reactor_tcp   tcp;
  reactor_http  http;
};

int client_event(void *state, int type, void *data)
{
  client *client = state;
  reactor_http_response *response;
  struct iovec *iov;

  switch (type)
    {
    case REACTOR_HTTP_EVENT_RESPONSE:
      response = data;
      (void) fprintf(stderr, "[response status %d]\n", response->status);
      (void) fflush(stderr);
      reactor_http_close(&client->http);
      break;
    case REACTOR_HTTP_EVENT_RESPONSE_HEADER:
      response = data;
      (void) fprintf(stderr, "[response status %d]\n", response->status);
      (void) fflush(stderr);
      break;
    case REACTOR_HTTP_EVENT_RESPONSE_DATA:
      iov = data;
      (void) fprintf(stderr, "[data]\n");
      (void) fflush(stderr);
      (void) fprintf(stdout, "%.*s", (int) iov->iov_len, (char *) iov->iov_base);
      (void) fflush(stdout);
      break;
    case REACTOR_HTTP_EVENT_CLOSE:
    case REACTOR_HTTP_EVENT_RESPONSE_END:
      (void) fprintf(stderr, "[end]\n");
      (void) fflush(stderr);
      reactor_http_close(&client->http);
      return 0;
    case REACTOR_HTTP_EVENT_ERROR:
      (void) fprintf(stderr, "[error]\n");
      (void) fflush(stderr);
      reactor_http_close(&client->http);
     break;
    }

  return REACTOR_OK;
}

int tcp_event(void *state, int type, void *data)
{
  client *client = state;
  char host[4096];
  int e;

  reactor_tcp_close(&client->tcp);
  if (type != REACTOR_TCP_EVENT_CONNECT)
    err(1, "tcp");

  (void) snprintf(host, sizeof host, "%s:%s", client->node, client->service);
  e = reactor_http_open(&client->http, client_event, client, *(int *) data, REACTOR_HTTP_FLAG_STREAM);
  if (e != REACTOR_OK)
    err(1, "reactor_http_open");
  reactor_http_send_request(&client->http, (reactor_http_request[]){{
        .method = {"GET", 3},
        .path = {client->path, strlen(client->path)},
        .headers = 1,
        .header[0] = {{"Host", 4}, {host, strlen(host)}},
        .version = 1,
      }});
  return reactor_http_flush(&client->http);
}

int main(int argc, char **argv)
{
  client client;
  int e;

  if (argc != 4)
    err(1, "Usage: http_client <ip> <port> <path>");

  client.node = argv[1];
  client.service = argv[2];
  client.path = argv[3];

  reactor_core_construct();
  e = reactor_tcp_open(&client.tcp, tcp_event, &client, client.node, client.service, 0);
  if (e != REACTOR_OK)
    err(1, "connect");
  reactor_core_run();
  reactor_core_destruct();
}
