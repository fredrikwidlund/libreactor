#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void test_timer(void **state)
{
  timer t;

  (void) state;
  reactor_construct();
  timer_construct(&t, NULL, NULL);
  timer_set(&t, 0, 0);
  reactor_loop_once();
  timer_set(&t, 1, 0);
  reactor_loop_once();
  timer_destruct(&t);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_timer)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
