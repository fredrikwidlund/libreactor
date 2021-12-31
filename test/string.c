#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#include <cmocka.h>

#include "reactor.h"

static void string_tests(void **arg)
{
  string s, t;
  data d;

  (void) arg;

  /* new string */
  s = string_null();
  assert_true(string_equal(string_null(), s));
  s = string_append_data(s, data_string("test"));
  string_free(s);

  /* load string from file and save */
  s = string_load("/dev/null");
  assert_true(string_equal(s, string_null()));
  assert_int_equal(string_save(s, "/dev/null"), 0);

  /* copy string */
  s = string_append_data(string_null(), data_string("test"));
  s = string_append_data(s, data_string("test"));
  string_free(string_copy(s));
  string_free(s);

  /* copy null string */
  string_free(string_copy(string_null()));

  /* read from file */
  string_free(string_load("LICENSE"));

  /* read from non-existing file */
  assert_true(string_equal(string_null(), string_load("/doesnotexist")));

  /* find sub-string */
  s = string_append_data(string_null(), data_string("xxxxxtestxxxxx"));
  d = string_find_data(s, data_string("xtestx"));
  assert_false(data_empty(d));

  /* fail find */
  d = string_find_data(s, data_string("ytesty"));
  assert_true(data_empty(d));

  /* fail null */
  d = string_find_data(s, data_null());
  assert_true(data_empty(d));
  string_free(s);

  /* compare */
  s = string_append_data(string_null(), data_string(""));
  assert_true(string_equal(string_null(), string_null()));
  assert_true(string_equal(s, string_null()));
  string_free(s);
  s = string_append_data(string_null(), data_string("a"));
  t = string_append_data(string_null(), data_string("b"));
  assert_false(string_equal(s, t));
  string_free(s);
  string_free(t);

  /* erase and replace null */
  s = string_append_data(string_null(), data_string("xxxxxtestxxxxx"));
  s = string_erase_data(s, data_null());
  s = string_replace_data(s, data_null(), data_null());
  assert_int_equal(string_size(s), 14);
  assert_int_equal(string_capacity(s), 14);
  string_free(s);

  /* prepend */
  s = string_prepend_data(string_null(), data_string("xxxxxtestxxxxx"));

  /* replace */
  s = string_replace_data(s, data_string("test"), data_null());
  s = string_replace_all_data(s, data_string("x"), data_string("y"));
  s = string_replace_all_data(s, data_string("x"), data_string("y"));
  assert_true(string_equal(s, s));
  assert_false(string_equal(s, string_null()));

  /* save */
  assert_int_equal(string_save(s, "doesnotexist"), -1);
  assert_int_equal(string_save(s, "/dev/null"), 0);
  string_free(s);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(string_tests)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
