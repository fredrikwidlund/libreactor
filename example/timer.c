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
  static int count = 10;
  reactor_timer *timer = state;

  (void) state;
  (void) data;
  if (type == REACTOR_TIMER_EVENT_CALL)
    {
      (void) fprintf(stderr, "%d\n", count);
      count --;
      if (!count)
        reactor_timer_close(timer);
    }
}

int main()
{
  reactor_timer timer;

  reactor_core_construct();
  reactor_timer_open(&timer, event, &timer, 1000000000, 1000000000);
  reactor_core_run();
  reactor_core_destruct();
}
