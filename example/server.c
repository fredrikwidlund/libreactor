#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <err.h>

#include <dynamic.h>

#include "reactor.h"

typedef struct client client;
struct client
{
  reactor_timer           timer;
  reactor_server_session *session;
};

static reactor_status server_route_fast(reactor_event *event)
{
  reactor_server_session *session = (reactor_server_session *) event->data;

  if (reactor_vector_equal(session->request->target, reactor_vector_string("/")))
    reactor_server_ok(session, reactor_vector_string("text/plain"), reactor_vector_string("Hello, World!"));
  return REACTOR_OK;
}

static void client_close(client *client)
{
  reactor_timer_destruct(&client->timer);
  free(client);
}

static reactor_status client_timer_handler(reactor_event *event)
{
  client *client = event->state;

  reactor_server_ok(client->session, reactor_vector_string("text/plain"), reactor_vector_string("hello world"));
  client_close(client);
  return REACTOR_ABORT;

}

static reactor_status client_session_handler(reactor_event *event)
{
  client *client = event->state;

  client_close(client);
  return REACTOR_ABORT;
}

static reactor_status server_route_slow(reactor_event *event)
{
  reactor_server_session *session = (reactor_server_session *) event->data;
  client *client;

  if (reactor_vector_equal_case(session->request->method, reactor_vector_string("QUIT")))
    {
      reactor_server_destruct(event->state);
      return REACTOR_ABORT;
    }

  if (!reactor_vector_equal(session->request->target, reactor_vector_string("/wait")))
    return REACTOR_OK;

  client = malloc(sizeof *client);
  if (!client)
    abort();

  reactor_timer_construct(&client->timer, client_timer_handler, client);
  reactor_timer_set(&client->timer, 1000000000, 0);
  client->session = session;
  reactor_server_register(session, client_session_handler, client);

  return REACTOR_OK;
}

static reactor_status server_handler(reactor_event *event)
{
  (void) fprintf(stderr, "server event: %d\n", event->type);
  return REACTOR_OK;
}

int main(int argc, char **argv)
{
  reactor_server server;
  char *node, *service;
  int e;

  node = argc >= 2 ? argv[1] : "127.0.0.1";
  service = argc >= 3 ? argv[2] : "80";

  signal(SIGPIPE, SIG_IGN);

  (void) reactor_construct();
  reactor_server_construct(&server, server_handler, &server);
  reactor_server_route(&server, server_route_fast, &server);
  reactor_server_route(&server, server_route_slow, &server);

  e = reactor_server_open(&server, node, service);
  if (e != REACTOR_OK)
    err(1, "reactor_server_open");

  reactor_run();
  reactor_destruct();
}
