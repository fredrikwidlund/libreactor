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

void event(void *state, int type, void *data)
{
  reactor_resolver *resolver = state;

  (void) resolver;
  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_ERROR:
      (void) fprintf(stderr, "error\n");
      break;
    case REACTOR_RESOLVER_EVENT_RESULT:
      (void) fprintf(stderr, "%sfound\n", data ? "" : "not ");
      break;
    }
}

int main(int argc, char **argv)
{
  reactor_resolver resolver;

  if (argc != 3)
    err(1, "usage: lookup node service");

  reactor_core_construct();
  reactor_resolver_open(&resolver, event, &resolver, argv[1], argv[2], NULL);
  reactor_core_run();
  reactor_core_destruct();
}
