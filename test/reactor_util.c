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

void core()
{
  char string[16];

  assert_int_equal(reactor_util_u32len(0), 1);
  assert_int_equal(reactor_util_u32len(10), 2);
  assert_int_equal(reactor_util_u32len(9999), 4);
  assert_int_equal(reactor_util_u32len(10000), 5);
  assert_int_equal(reactor_util_u32len(4294967295), 10);

  reactor_util_u32toa(0, string);
  assert_string_equal(string, "0");
  reactor_util_u32toa(100, string);
  assert_string_equal(string, "100");
  reactor_util_u32toa(4294967295, string);
  assert_string_equal(string, "4294967295");
}


int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
