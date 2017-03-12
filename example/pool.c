#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>
#include <reactor.h>

void event(void *state, int type, void *data)
{
  (void) state;
  (void) data;
  switch (type)
    {
    case REACTOR_POOL_EVENT_CALL:
      sleep(1);
      break;
    case REACTOR_POOL_EVENT_RETURN:
      break;
    }
}

int main()
{
  reactor_pool pool;
  int i, j;

  for (i = 0; i < 60; i ++)
    {
      reactor_core_construct();
      reactor_pool_construct(&pool);
      for (j = 0; j < 1000; j ++)
        reactor_pool_enqueue(&pool, event, NULL);
      reactor_core_run();
      reactor_pool_destruct(&pool);
      reactor_core_destruct();
    }
}
