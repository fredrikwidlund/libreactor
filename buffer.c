#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>

#include "buffer.h"

/* allocators */

buffer *buffer_new()
{
  buffer *b;

  b = malloc(sizeof *b);
  if (!b)
    return NULL;

  buffer_init(b);

  return b;
}

void buffer_free(buffer *b)
{
  buffer_clear(b);
  free(b);
}

void buffer_init(buffer *b)
{
  b->data = NULL;
  b->size = 0;
  b->capacity = 0;
}

char *buffer_deconstruct(buffer *b)
{
  char *data;

  (void) buffer_compact(b);
  data = buffer_data(b);
  free(b);

  return data;
}

/* capacity */

size_t buffer_size(buffer *b)
{
  return b->size;
}

size_t buffer_capacity(buffer *b)
{
  return b->capacity;
}

int buffer_reserve(buffer *b, size_t capacity)
{
  char *data;

  if (capacity > b->capacity)
    {
      capacity = buffer_roundup(capacity);
      data = realloc(b->data, capacity);
      if (!data)
        return -1;
      b->data = data;
      b->capacity = capacity;
    }

  return 0;
}

int buffer_compact(buffer *b)
{
  char *data;

  if (b->capacity > b->size)
    {
      data = realloc(b->data, b->size);
      if (!data)
        return -1;
      b->data = data;
      b->capacity = b->size;
    }

  return 0;
}

/* modifiers */

int buffer_insert(buffer *b, size_t position, char *data, size_t size)
{
  int e;

  e = buffer_reserve(b, b->size + size);
  if (e == -1)
    return -1;

  if (position < b->size)
    memmove(b->data + position + size, b->data + position, b->size - position);
  memcpy(b->data + position, data, size);
  b->size += size;

  return 0;
}

void buffer_erase(buffer *b, size_t position, size_t size)
{
  memmove(b->data + position, b->data + position + size, b->size - position - size);
  b->size -= size;
}

void buffer_clear(buffer *b)
{
  free(b->data);
  buffer_init(b);
}

/* element access */

char *buffer_data(buffer *b)
{
  return b->data;
}

/* internals */

size_t buffer_roundup(size_t size)
{
  size --;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size |= size >> 32;
  size ++;

  return size;
}
