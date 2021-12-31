#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "reactor.h"

void test_mapi(__attribute__((unused)) void **state)
{
  mapi mapi;

  mapi_construct(&mapi);

  mapi_insert(&mapi, 1, 42, NULL);
  mapi_insert(&mapi, 1, 42, NULL);
  assert_true(mapi_at(&mapi, 1) == 42);
  assert_true(mapi_size(&mapi) == 1);
  mapi_erase(&mapi, 2, NULL);
  assert_true(mapi_size(&mapi) == 1);
  mapi_erase(&mapi, 1, NULL);
  assert_true(mapi_size(&mapi) == 0);

  mapi_reserve(&mapi, 32);
  assert_true(mapi.map.elements_capacity == 64);
  mapi_clear(&mapi, NULL);
  mapi_destruct(&mapi, NULL);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_mapi)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
