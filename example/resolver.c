#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <err.h>
#include <dynamic.h>

#include "reactor_core.h"

typedef struct client client;
struct client
{
  reactor_resolver resolver;
};

void event(void *state, int type, void *data)
{
  client *client = state;

  printf("event %d\n", type);
  switch (type)
    {
    case REACTOR_RESOLVER_SIGNAL:
      break;
    case REACTOR_RESOLVER_ERROR:
      break;
    }
}

int main(int argc, char **argv)
{
  client client = {0};

  if (argc != 3)
    errx(1, "usage: resolver host service\n");
  reactor_core_open();
  reactor_resolver_init(&client.resolver, event, &client);
  reactor_resolver_open(&client.resolver, argv[1], argv[2], NULL);
  assert(reactor_core_run() == 0);
  reactor_core_close();
}
