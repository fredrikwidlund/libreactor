#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "reactor.h"

static void test_utility(__attribute__((unused)) void **arg)
{
  uint64_t t1, t2;
  char buf[256];

  t1 = utility_tsc();
  t2 = utility_tsc();
  assert_true((t1 == 0 && t2 == 0) || t1 != t2);

  utility_u32_toa(1, buf);
  assert_int_equal(strcmp(buf, "1"), 0);

  utility_u32_toa(12, buf);
  assert_int_equal(strcmp(buf, "12"), 0);

  utility_u32_toa(4294967295, buf);
  assert_int_equal(strcmp(buf, "4294967295"), 0);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_utility)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
