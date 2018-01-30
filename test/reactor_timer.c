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

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"


extern int mock_timerfd_failure;
extern int mock_read_failure;
extern int mock_epoll_ctl_failure;

static int basic_error = 0;

int basic_event(void *state, int type, void *data)
{
  reactor_timer *timer = state;
  uint64_t *expirations;

  printf("type %d\n", type);
  switch (type)
    {
    case REACTOR_TIMER_EVENT_CALL:
      expirations = data;
      assert_true(*expirations == 1);
      reactor_timer_close(timer);
      break;
    case REACTOR_TIMER_EVENT_ERROR:
      basic_error ++;
      reactor_timer_close(timer);
      break;
    }

  return REACTOR_ABORT;
}

void basic()
{
  reactor_timer timer;
  int e;

  // timer
  reactor_core_construct();
  reactor_timer_open(&timer, basic_event, &timer, 1000000000, 1000000000);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();

  // double close
  reactor_core_construct();
  reactor_timer_open(&timer, basic_event, &timer, 1000000000, 1000000000);
  reactor_timer_close(&timer);
  reactor_timer_close(&timer);
  reactor_core_destruct();
}

void timerfd_error()
{
  reactor_timer timer;
  uint64_t t = 100000000;
  int e;

  // timerfd_create error
  mock_timerfd_failure = 1;
  reactor_core_construct();
  e = reactor_timer_open(&timer, basic_event, &timer, t, t);
  assert_int_equal(e, REACTOR_ERROR);
  reactor_core_destruct();

  // timerfd_settime error
  reactor_core_construct();
  e = reactor_timer_open(&timer, basic_event, &timer, t, t);
  assert_int_equal(e, REACTOR_OK);
  mock_timerfd_failure = 1;
  e = reactor_timer_set(&timer, t, t);
  assert_int_equal(e, REACTOR_ERROR);
  reactor_core_destruct();

  // timerfd read error
  mock_read_failure = 1;
  reactor_core_construct();
  e = reactor_timer_open(&timer, basic_event, &timer, t, t);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();
  assert_int_equal(basic_error, 1);
  basic_error = 0;

  // epoll ctl error
  mock_epoll_ctl_failure = 1;
  reactor_core_construct();
  e = reactor_timer_open(&timer, basic_event, &timer, t, t);
  assert_int_equal(e, REACTOR_ERROR);
  mock_epoll_ctl_failure = 0;
  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(basic),
    cmocka_unit_test(timerfd_error)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
