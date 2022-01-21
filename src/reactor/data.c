#include <string.h>
#include <stdlib.h>

#include "data.h"

data data_alloc(size_t size)
{
  return data_construct(calloc(1, size), size);
}

void data_free(data data)
{
  free((void *) data_base(data));
}

data data_construct(const void *base, size_t size)
{
  return (data) {.base = base, .size = size};
}

data data_select(data data, size_t offset, size_t size)
{
  return data_construct((char *) data_base(data) + offset, size);
}

data data_null(void)
{
  return (data) {0};
}

data data_string(const char *chars)
{
  return data_construct(chars, strlen(chars));
}

const void *data_base(data data)
{
  return data.base;
}

size_t data_size(data data)
{
  return data.size;
}

int data_empty(data data)
{
  return data_size(data) == 0;
}

int data_equal(data d1, data d2)
{
  return data_size(d1) == data_size(d2) && memcmp(data_base(d1), data_base(d2), data_size(d1)) == 0;
}

int data_prefix(data d1, data d2)
{
  return data_size(d1) <= data_size(d2) && memcmp(data_base(d1), data_base(d2), data_size(d1)) == 0;
}

size_t data_offset(data d1, data d2)
{
  return (uintptr_t) data_base(d2) - (uintptr_t) data_base(d1);
}

data data_consume(data data, size_t size)
{
  return data_construct((char *) data.base + size, data.size - size);
}

data data_merge(data d1, data d2)
{
  return data_construct(data_base(d1), data_offset(d1, d2) + data_size(d2));
}
