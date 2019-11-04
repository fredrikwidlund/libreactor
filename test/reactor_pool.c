#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static int event(reactor_event *event)
{
  int *n = event->state;

  switch (event->type)
    {
    case REACTOR_POOL_EVENT_CALL:
      (*n)--;
      break;
    case REACTOR_POOL_EVENT_RETURN:
      assert_int_equal(*n, 41);
      break;
    }

  return REACTOR_OK;
}

static void core()
{
  int n = 42;

  /* dispatch pool event */
  reactor_construct();
  reactor_pool_dispatch(event, &n);
  reactor_run();
  reactor_destruct();

  /* construct and destruct */
  reactor_pool_construct();
  reactor_pool_construct();
  reactor_pool_destruct();
  reactor_pool_destruct();
  reactor_pool_destruct();
}

struct stress_state
{
  reactor_timer timer;
  int           target;
  int           dispatches;
  int           returns;
  _Atomic int   calls;
};

static int stress_event(reactor_event *event)
{
  struct stress_state *state = event->state;

  switch (event->type)
    {
    case REACTOR_POOL_EVENT_CALL:
      state->calls ++;
      break;
    case REACTOR_POOL_EVENT_RETURN:
      state->returns ++;
      if (state->returns == state->target)
        {
          assert_int_equal(state->dispatches, state->target);
          assert_int_equal(state->calls, state->target);
          reactor_timer_destruct(&state->timer);
          free(state);
          return REACTOR_ABORT;
        }
      break;
    }
  return REACTOR_OK;
}

static int stress_timer(reactor_event *event)
{
  struct stress_state *state = event->state;

  if (state->dispatches < state->target)
    {
      reactor_pool_dispatch(stress_event, state);
      state->dispatches ++;
    }
  return REACTOR_OK;
}

static void stress_new(int target)
{
  struct stress_state *state;

  state = calloc(1, sizeof *state);
  if (!state)
    abort();

  state->target = target;
  reactor_timer_construct(&state->timer, stress_timer, state);
  reactor_timer_set(&state->timer, 1000000, 1000000);
}

static void stress()
{
  int i;

  reactor_construct();
  for (i = 0; i < 100; i ++)
    stress_new(100);
  reactor_run();
  reactor_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(core),
     cmocka_unit_test(stress)
    };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
