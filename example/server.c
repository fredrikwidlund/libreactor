#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

static core_status handler(core_event *event)
{
  server *server = event->state;
  server_context *context = (server_context *) event->data;

  switch (event->type)
    {
    case SERVER_REQUEST:
       server_ok(context, segment_string("text/plain"), segment_string("Hello, World!"));
       return CORE_OK;
    default:
      warn("error");
      server_destruct(server);
      return CORE_ABORT;
    }
}

int main()
{
  server s;

  signal(SIGPIPE, SIG_IGN);
  core_construct(NULL);
  server_construct(&s, handler, &s);
  server_open(&s, 0, 80);
  core_loop(NULL);
  core_destruct(NULL);
}
