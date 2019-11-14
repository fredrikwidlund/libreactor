#include <stdio.h>
#include <stdint.h>
#include <dynamic.h>
#include <reactor.h>

static reactor_status hello(reactor_event *event)
{
  reactor_server_session *session = (reactor_server_session *) event->data;
  reactor_server_ok(session, reactor_vector_string("text/plain"), reactor_vector_string("Hello, World!"));
  return REACTOR_OK;
}

int main()
{
  reactor_server server;
  reactor_construct();
  reactor_server_construct(&server, NULL, NULL);
  reactor_server_route(&server, hello, NULL);
  (void) reactor_server_open(&server, "0.0.0.0", "8080");
  reactor_run();
  reactor_destruct();
}
