#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor_core.h"

extern int debug_io_error;

static int timer_called = 0;

void event(void *state, int type, void *data)
{
  uint64_t *expirations;

  assert_true(state != NULL);
  if (type == REACTOR_TIMER_TIMEOUT)
    {
      expirations = data;
      timer_called += *expirations;
      if (timer_called >= 5)
        reactor_timer_close(state);
    }
}

void coverage()
{
  reactor_timer timer;
  int e;

  reactor_timer_init(&timer, event, &timer);

  /* timerfd error */
  debug_io_error = 1;
  e = reactor_timer_open(&timer, 1000000, 1000000);
  assert_int_equal(e, -1);
  debug_io_error = 0;

  /* open with constructed reactor */
  e = reactor_timer_open(&timer, 1000000, 1000000);
  assert_int_equal(e, -1);

  /* attempt to set interval on closed timer */
  e = reactor_timer_set(&timer, 1000000, 1000000);
  assert_int_equal(e, -1);

  /* attempt to close already closed timer */
  reactor_timer_close(&timer);

  /* simulate timer event on closed timer */
  reactor_timer_event(&timer, REACTOR_DESC_READ, NULL);

  reactor_core_construct();

  /* successful open */
  e = reactor_timer_open(&timer, 1000000, 1000000);
  assert_int_equal(e, 0);

  /* attempt to open already open timer */
  e = reactor_timer_open(&timer, 1000000, 1000000);
  assert_int_equal(e, -1);

  /* simulate io error on simulated timer event */
  debug_io_error = 1;
  reactor_timer_event(&timer, REACTOR_DESC_READ, NULL);
  debug_io_error = 0;

  /* simulate unknown event type */
  reactor_timer_event(&timer, -1, NULL);

  /* run reactor */
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(timer_called >= 5);

  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
