#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <dynamic.h>

#include "../reactor.h"

size_t reactor_utility_u32len(uint32_t n)
{
  static const uint32_t pow_10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  uint32_t t;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  return t - (n < pow_10[t]) + 1;
}

void reactor_utility_u32sprint(uint32_t n, char *string)
{
  static const char digits[] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";
  size_t i;

  while (n >= 100)
    {
      i = (n % 100) << 1;
      n /= 100;
      *--string = digits[i + 1];
      *--string = digits[i];
    }

  if (n < 10)
    {
      *--string = n + '0';
    }
  else
    {
      i = n << 1;
      *--string = digits[i + 1];
      *--string = digits[i];
    }
}

void reactor_utility_u32toa(uint32_t n, char *string)
{
  size_t length;

  length = reactor_utility_u32len(n);
  string += length;
  *string = 0;
  reactor_utility_u32sprint(n, string);
}

reactor_vector reactor_utility_u32tov(uint32_t n)
{
  static __thread char string[16];
  size_t length;
  char *s = string;

  length = reactor_utility_u32len(n);
  s += length;
  *s = 0;
  reactor_utility_u32sprint(n, s);

  return (reactor_vector){string, length};
}
