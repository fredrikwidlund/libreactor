#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void test_callback(reactor_event *reactor_event)
{
  int *called = (int *) reactor_event->state;

  assert(reactor_event->type == EVENT_SIGNAL);
  *called = 1;
}

static void test_event(__attribute__((unused)) void **state)
{
  event e;
  int called = 0;

  reactor_construct();
  event_construct(&e, test_callback, &called);
  event_open(&e);
  event_signal(&e);
  reactor_loop_once();
  assert_int_equal(called, 1);
  event_close(&e);
  event_destruct(&e);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
      {
        cmocka_unit_test(test_event)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
