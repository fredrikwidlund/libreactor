#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static int next(reactor_event *event)
{
  int *n = event->state;

  (*n)++;
  return REACTOR_OK;
}

static void core()
{
  int n = 0;
  uint64_t now;
  reactor_id id;

  /* construct and destruct */
  reactor_construct();
  reactor_construct();
  reactor_destruct();
  reactor_destruct();
  reactor_destruct();

  /* run empty core */
  reactor_construct();
  reactor_run();
  reactor_destruct();

  /* add, modify and remove fd */
  reactor_construct();
  reactor_core_add(NULL, 0, 0);
  reactor_core_modify(NULL, 0, EPOLLIN);
  reactor_core_delete(NULL, 0);
  reactor_run();
  reactor_destruct();

  /* schedule event for next tick */
  reactor_construct();
  reactor_core_schedule(next, &n);
  assert_int_equal(n, 0);
  reactor_run();
  assert_int_equal(n, 1);
  reactor_destruct();

  /* schedule and cancel event */
  n = 0;
  reactor_construct();
  id = reactor_core_schedule(next, &n);
  assert_int_equal(n, 0);
  reactor_core_cancel(id);
  reactor_run();
  assert_int_equal(n, 0);
  reactor_core_cancel(0);
  reactor_destruct();

  /* get time from reactor */
  reactor_construct();
  now = reactor_core_now();
  assert_true(now > 0);
  reactor_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(core),
    };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
