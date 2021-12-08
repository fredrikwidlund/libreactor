#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"
#include "vector.h"

/* constructor/destructor */

void vector_construct(vector *v, size_t object_size)
{
  buffer_construct(&v->buffer);
  v->object_size = object_size;
}

void vector_destruct(vector *v, vector_release *release)
{
  vector_clear(v, release);
}

/* capacity */

size_t vector_size(vector *v)
{
  return buffer_size(&v->buffer) / v->object_size;
}

size_t vector_capacity(vector *v)
{
  return buffer_capacity(&v->buffer) / v->object_size;
}

int vector_empty(vector *v)
{
  return vector_size(v) == 0;
}

void vector_reserve(vector *v, size_t capacity)
{
  buffer_reserve(&v->buffer, capacity * v->object_size);
}

void vector_shrink_to_fit(vector *v)
{
  buffer_compact(&v->buffer);
}

/* element access */

void *vector_at(vector *v, size_t position)
{
  return (char *) buffer_data(&v->buffer) + (position * v->object_size);
}

void *vector_front(vector *v)
{
  return vector_data(v);
}

void *vector_back(vector *v)
{
  return (char *) buffer_data(&v->buffer) + buffer_size(&v->buffer) - v->object_size;
}

void *vector_data(vector *v)
{
  return buffer_data(&v->buffer);
}

/* modifiers */

void vector_insert(vector *v, size_t position, void *object)
{
  buffer_insert(&v->buffer, position * v->object_size, object, v->object_size);
}

void vector_insert_range(vector *v, size_t position, void *first, void *last)
{
  buffer_insert(&v->buffer, position * v->object_size, first, (char *) last - (char *) first);
}

void vector_insert_fill(vector *v, size_t position, size_t count, void *object)
{
  buffer_insert_fill(&v->buffer, position * v->object_size, count, object, v->object_size);
}

void vector_erase(vector *v, size_t position, vector_release *release)
{
  if (release)
    release(vector_at(v, position));

  buffer_erase(&v->buffer, position * v->object_size, v->object_size);
}

void vector_erase_range(vector *v, size_t first, size_t last, vector_release *release)
{
  size_t i;

  if (release)
    for (i = first; i < last; i++)
      release(vector_at(v, i));

  buffer_erase(&v->buffer, first * v->object_size, (last - first) * v->object_size);
}

void vector_clear(vector *v, vector_release *release)
{
  vector_erase_range(v, 0, vector_size(v), release);
  buffer_clear(&v->buffer);
}

void vector_push_back(vector *v, void *object)
{
  buffer_insert(&v->buffer, buffer_size(&v->buffer), object, v->object_size);
}

void vector_pop_back(vector *v, vector_release *release)
{
  vector_erase(v, vector_size(v) - 1, release);
}
