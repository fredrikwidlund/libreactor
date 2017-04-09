#include <string.h>

#include "reactor_memory.h"

reactor_memory reactor_memory_ref(const char *base, size_t size)
{
  return (reactor_memory){base, size};
}

reactor_memory reactor_memory_str(const char *str)
{
  return reactor_memory_ref(str, strlen(str));
}

const char *reactor_memory_base(reactor_memory m)
{
  return m.base;
}

size_t reactor_memory_size(reactor_memory m)
{
  return m.size;
}

int reactor_memory_empty(reactor_memory m)
{
  return reactor_memory_base(m) == NULL;
}

int reactor_memory_equal(reactor_memory m1, reactor_memory m2)
{
  return reactor_memory_size(m1) == reactor_memory_size(m2) && memcmp(reactor_memory_base(m1), reactor_memory_base(m2), reactor_memory_size(m1)) == 0;
}

int reactor_memory_equal_case(reactor_memory m1, reactor_memory m2)
{
  return reactor_memory_size(m1) == reactor_memory_size(m2) && strncasecmp(reactor_memory_base(m1), reactor_memory_base(m2), reactor_memory_size(m1)) == 0;
}
