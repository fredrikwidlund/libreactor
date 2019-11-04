#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

struct server
{
  reactor_net    net;
  list           connections;
};

struct connection
{
  struct server *server;
  reactor_http   http;
};

static void quit(struct server *server)
{
  struct connection *c;

  while (!list_empty(&server->connections))
    {
      c = list_front(&server->connections);
      reactor_http_destruct(&c->http);
      list_erase(c, NULL);
    }
  reactor_net_destruct(&server->net);
}

static reactor_status http_handler(reactor_event *event)
{
  struct connection *connection = event->state;
  reactor_http_request *request = (reactor_http_request *) event->data;
  reactor_http_response response;

  switch (event->type)
    {
    case REACTOR_HTTP_EVENT_REQUEST:
      if (reactor_vector_equal(request->method, reactor_vector_string("QUIT")))
        {
          quit(connection->server);
          return REACTOR_ABORT;
        }
      reactor_http_create_response(&connection->http, &response, 1, 200, reactor_vector_string("OK"),
                                   reactor_vector_string("Text/Plain"), 18, reactor_vector_string("Hello World!\n"));
      reactor_http_write_response(&connection->http, &response);
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_REQUEST_HEAD:
      if (reactor_vector_equal(request->method, reactor_vector_string("QUIT")))
        {
          quit(connection->server);
          return REACTOR_ABORT;
        }
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_REQUEST_BODY:
      reactor_http_create_response(&connection->http, &response, 1, 200, reactor_vector_string("OK"),
                                   reactor_vector_string("Text/Plain"), 13, reactor_vector_string("Hello World!\n"));
      reactor_http_write_response(&connection->http, &response);
      return REACTOR_OK;
    default:
      reactor_http_destruct(&connection->http);
      list_erase(connection, NULL);
      return REACTOR_ABORT;
    }
}

static reactor_status net_handler(reactor_event *event)
{
  struct server *server = event->state;
  struct connection connection = {0}, *c;

  switch (event->type)
    {
    case REACTOR_NET_EVENT_ACCEPT:
      list_push_back(&server->connections, &connection, sizeof connection);
      c = list_back(&server->connections);
      c->server = server;
      reactor_http_construct(&c->http, http_handler, c);
      reactor_http_set_mode(&c->http, REACTOR_HTTP_MODE_REQUEST_STREAM);
      reactor_http_open(&c->http, event->data);
      return REACTOR_OK;
    default:
      reactor_net_destruct(&server->net);
      return REACTOR_ABORT;
    }
}

int main(int argc, char **argv)
{
  struct server server = {0};
  char *node, *service;
  int e;

  node = argc >= 2 ? argv[1] : "127.0.0.1";
  service = argc >= 3 ? argv[2] : "80";
  list_construct(&server.connections);

  (void) reactor_construct();
  reactor_net_construct(&server.net, net_handler, &server);

  e = reactor_net_bind(&server.net, node, service);
  if (e != REACTOR_OK)
    err(1, "reactor_net_connect");

  reactor_run();
  reactor_destruct();
}
