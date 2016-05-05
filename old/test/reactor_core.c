#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor_core.h"

extern int debug_epoll_error;

static int called = 0;

void call(void *state, int type, void *data)
{
  if (type == REACTOR_TIMER_TIMEOUT)
    {
      assert_true(state);
      assert_true(data);
      reactor_timer_close(state);
      called = 1;
    }
}

void coverage()
{
  reactor_timer timer;
  int e;

  debug_epoll_error = 1;
  e = reactor_core_construct();
  assert_int_equal(e, -1);

  debug_epoll_error = 0;
  e = reactor_core_construct();
  assert_int_equal(e, 0);
  e = reactor_core_construct();
  assert_int_equal(e, 0);

  reactor_timer_init(&timer, call, &timer);
  e = reactor_timer_open(&timer, 1, 0);
  assert_int_equal(e, 0);

  debug_epoll_error = 1;
  e = reactor_core_run();
  assert_int_equal(e, -1);

  debug_epoll_error = 0;
  e = reactor_core_run();
  assert_int_equal(e, 0);

  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
