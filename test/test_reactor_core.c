#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <signal.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

void coverage()
{
  int e;

  e = reactor_core_open();
  assert_int_equal(e, 0);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_close();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
