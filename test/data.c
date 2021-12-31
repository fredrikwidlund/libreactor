#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "reactor.h"

static void test_data(__attribute__((unused)) void **arg)
{
  data d, e;

  /* allocate */
  d = data_alloc(1024);
  assert_int_equal(data_size(d), 1024);
  data_free(d);

  d = data_null();
  assert_true(data_empty(d));

  /* prefix */
  d = data_string("test");
  assert_true(strcmp(data_base(d), "test") == 0);
  assert_true(data_prefix(data_string("te"), d));
  assert_false(data_prefix(data_string("tet"), d));
  assert_false(data_prefix(data_string("test2"), d));

  /* select */
  e = data_select(d, 2, 2);
  assert_int_equal(data_offset(d, e), 2);

  /* compare */
  assert_true(data_equal(data_string("test"), data_string("test")));
  assert_false(data_equal(data_string("test1"), data_string("test2")));
  assert_false(data_equal(data_string("test"), data_string("test2")));
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_data)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
