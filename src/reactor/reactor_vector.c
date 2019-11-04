#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

reactor_vector reactor_vector_data(void *base, size_t size)
{
  return (reactor_vector){.base = base, .size = size};
}

reactor_vector reactor_vector_string(char *string)
{
  if (!string)
    return reactor_vector_empty();
  return (reactor_vector){.base = string, .size = strlen(string)};
}

reactor_vector reactor_vector_empty(void)
{
  return (reactor_vector){0};
}

int reactor_vector_equal(reactor_vector v1, reactor_vector v2)
{
  return v1.size == v2.size && memcmp(v1.base, v2.base, v2.size) == 0;
}

int reactor_vector_equal_case(reactor_vector v1, reactor_vector v2)
{
  return v1.size == v2.size && strncasecmp(v1.base, v2.base, v2.size) == 0;
}

reactor_vector reactor_vector_copy(reactor_vector source)
{
  reactor_vector destination;

  destination.size = source.size;
  if (destination.size)
    {
      destination.base = malloc(destination.size);
      if (!destination.base)
        abort();
      memcpy(destination.base, source.base, destination.size);
    }
  else
    destination.base = NULL;

  return destination;
}

void reactor_vector_clear(reactor_vector vector)
{
  free(vector.base);
}
