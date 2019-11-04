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

extern int debug_abort;

static void core()
{
  reactor_assert_int_equal(0, 0);
  reactor_assert_int_not_equal(0, 1);

  debug_abort = 1;
  expect_assert_failure(reactor_assert_int_equal(0, 1));
  expect_assert_failure(reactor_assert_int_not_equal(0, 0));
  debug_abort = 0;
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
