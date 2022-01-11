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

  switch (event->type)
  {
  case ASYNC_CALL:
    *called = 1;
    break;
  case ASYNC_DONE:
    assert_int_equal(*called, 1);
    break;
  }
}

static void test_async(__attribute__((unused)) void **state)
{
  async async;
  int called;

  called = 0;
  reactor_construct();
  async_construct(&async, test_callback, &called);
  async_call(&async);
  reactor_loop_once();
  assert_int_equal(called, 1);
  async_cancel(&async);
  async_destruct(&async);
  reactor_destruct();

  called = 0;
  reactor_construct();
  async_construct(&async, test_callback, &called);
  async_call(&async);
  async_cancel(&async);
  async_destruct(&async);
  reactor_destruct();
  usleep(1000);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
        cmocka_unit_test(test_async)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
