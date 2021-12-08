#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>

#include "dynamic.h"

size_t utility_u32_len(uint32_t n)
{
  static const uint32_t pow_10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  uint32_t t;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  return t - (n < pow_10[t]) + 1;
}

void utility_u32_sprint(uint32_t n, char *string)
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

void utility_u32_toa(uint32_t n, char *string)
{
  size_t length;

  length = utility_u32_len(n);
  string += length;
  *string = 0;
  utility_u32_sprint(n, string);
}

uint64_t utility_tsc(void)
{
#if defined(__x86_64__) || defined(__amd64__)
  uint32_t lo, hi;
  __asm__ volatile("RDTSC"
                   : "=a"(lo), "=d"(hi));
  return (((uint64_t) hi) << 32) | lo;
#else
  return 0;
#endif
}
