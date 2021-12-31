#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <string.h>

#include <cmocka.h>

#include "reactor.h"

static void pointer_tests(void **arg)
{
  char buffer[16]; 
  pointer p = buffer;

  (void) arg;

  pointer_push(&p, data_string("test"));
  pointer_push_byte(&p, 0);
  pointer_move(&p, -5);
  assert_int_equal(strcmp(p, "test"), 0);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(pointer_tests)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
