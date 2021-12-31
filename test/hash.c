#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>

#include "reactor.h"

void test_hash(__attribute__((unused)) void **state)
{
  int i;
  struct
  {
    char *in;
    uint64_t out;
  } strings[] =
      {
          {"", 0x9ae16a3b2f90404f},
          {"1", 0x811f023a122d0be1},
          {"1234", 0xc3fa0b46e8adcae},
          {"12345678", 0x2f99d2664a0fb6ea},
          {"1234567890123456", 0xef3d9afd22778424},
          {"12345678901234567890123456789012", 0xf8317d59683e31b1},
          {"1234567890123456789012345678901234567890123456789012345678901234", 0x78a95a0d9788b255},
          {"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012"
           "3456789012345678",
           0x6085a2475352e7f9},
          {"1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012"
           "34567890123456789",
           0xed4501398023b759},
          {NULL, 0}};
  struct
  {
    uint64_t in;
    uint64_t out;
  } integers[] =
      {
          {1ULL, 12994781566227106604ULL},
          {12994781566227106604ULL, 9038243705893100514ULL},
          {0, 0}};

  for (i = 0; strings[i].in; i++)
    assert_true(hash_string(strings[i].in) == strings[i].out);

  for (i = 0; integers[i].in; i++)
    assert_true(hash_uint64(integers[i].in) == integers[i].out);
}

int main()
{
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_hash)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
