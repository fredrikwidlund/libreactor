#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor_core.h"

static int called = 0;

void call(void *state, int type, void *data)
{
  assert_true(state == NULL);
  assert_int_equal(type, 0);
  assert_true(data == NULL);

  called = 1;
}

void coverage()
{
  reactor_user user;

  reactor_user_init(&user, call, NULL);
  reactor_user_dispatch(&user, 0, NULL);
  assert_int_equal(called, 1);
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
