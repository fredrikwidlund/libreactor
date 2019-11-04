#include <stdio.h>
#include <stdint.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

static reactor_status route(reactor_event *event)
{
  reactor_server_ok((reactor_server_session *) event->data,
                    reactor_vector_string("text/plain"),
                    reactor_vector_string("Hello, World!"));
  return REACTOR_OK;
}

int main()
{
  reactor_server server;
  int e;

  reactor_construct();

  reactor_server_construct(&server, NULL, NULL);
  reactor_server_route(&server, route, NULL);
  e = reactor_server_open(&server, "0.0.0.0", "8080");
  if (e != REACTOR_OK)
    err(1, "reactor_server_open");

  reactor_run();
  reactor_destruct();
}
