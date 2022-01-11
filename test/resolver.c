#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void test_callback(reactor_event *event)
{
  int *called = (int *) event->state;

  *called = 1;
}

static void test_one(char *host, char *service)
{
  resolver resolver;
  int called;

  called = 0;
  reactor_construct();
  resolver_construct(&resolver, test_callback, &called);
  resolver_lookup(&resolver, host, service, 0, 0, 0, 0);
  reactor_loop_once();
  assert_int_equal(called, 1);
  resolver_destruct(&resolver);
  reactor_destruct();
}

static void test_resolver(__attribute__((unused)) void **state)
{
  test_one(NULL, NULL);
  test_one("localhost", "http");
}

int main()
{
  const struct CMUnitTest tests[] =
      {
        cmocka_unit_test(test_resolver)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
