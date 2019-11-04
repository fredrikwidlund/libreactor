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

  assert_int_equal(*n, 42);
  switch (event->type)
    {
    case REACTOR_RESOLVER_EVENT_RESPONSE:
      assert_true(event->data != 0);
      return REACTOR_ABORT;
    default:
      return REACTOR_OK;
    }
}

static void core()
{
  int n = 42;

  reactor_construct();
  assert_true(reactor_resolver_request(event, &n, "localhost", "http", 0, 0, 0));
  reactor_destruct();

  reactor_construct();
  assert_true(reactor_resolver_request(event, &n, "localhost", "http", 0, 0, 0));
  reactor_run();
  reactor_destruct();

  assert_int_equal(reactor_resolver_request(event, &n, "localhost", "http", 0, 0, 0), 0);
}

static void concurrency()
{
  int i, n = 42;

  reactor_construct();
  for (i = 0; i < 32; i ++)
    assert_true(reactor_resolver_request(event, &n, "127.0.0.1", "http", 0, 0, 0));
  reactor_run();
  reactor_destruct();
}

static void aborts()
{
  int i, n = 42;
  reactor_id id[32];

  reactor_construct();
  for (i = 0; i < 32; i ++)
  {
    id[i] = reactor_resolver_request(event, &n, "localhost", "http", 0, 0, 0);
    assert_true(id[i] != 0);
  }
  for (i = 0; i < 32; i ++)
    reactor_resolver_cancel(id[i]);
  n = 0;
  reactor_run();

  reactor_resolver_cancel(0);
  reactor_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(core),
     cmocka_unit_test(concurrency),
     cmocka_unit_test(aborts)
    };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
