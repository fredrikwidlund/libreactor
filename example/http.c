#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

struct client
{
  reactor_net    net;
  reactor_http   http;
  reactor_vector target;
};

static reactor_status net_handler(reactor_event *event)
{
  struct client *client = event->state;

  if (event->type == REACTOR_NET_EVENT_CONNECT)
    {
      reactor_http_open(&client->http, event->data);
      reactor_http_get(&client->http, client->target);
    }

  reactor_net_destruct(&client->net);
  return REACTOR_ABORT;
}

static reactor_status http_handler(reactor_event *event)
{
  struct client *client = event->state;
  reactor_http_response *response = (reactor_http_response *) event->data;

  if (event->type == REACTOR_HTTP_EVENT_RESPONSE)
    {
      (void) fprintf(stdout, "[status] %d\n", response->code);
      (void) fprintf(stdout, "%.*s", (int) response->body.size, (char *) response->body.base);
    }

  reactor_http_destruct(&client->http);
  return REACTOR_ABORT;
}

int main(int argc, char **argv)
{
  struct client client = {0};
  char *node, *service;
  int e;

  node = argc >= 2 ? argv[1] : "127.0.0.1";
  service = argc >= 3 ? argv[2] : "80";
  client.target = reactor_vector_string(argc >= 4 ? argv[3] : "/");

  (void) reactor_construct();
  reactor_net_construct(&client.net, net_handler, &client);
  reactor_http_construct(&client.http, http_handler, &client);
  reactor_http_set_authority(&client.http, reactor_vector_string(node), reactor_vector_string(service));

  e = reactor_net_connect(&client.net, node, service);
  if (e != REACTOR_OK)
    err(1, "reactor_net_connect");
  (void) reactor_run();

  reactor_destruct();
}
