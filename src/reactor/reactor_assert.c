#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dynamic.h>

#include "../reactor.h"

void reactor_assert_int_equal(int a, int b)
{
  if (a != b)
    abort();
}

void reactor_assert_int_not_equal(int a, int b)
{
  if (a == b)
    abort();
}
