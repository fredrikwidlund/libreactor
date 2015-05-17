#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "vector.h"

/* allocators */

vector *vector_new(size_t object_size)
{
  vector *v;
  
  v = malloc(sizeof *v);
  if (!v)
    return NULL;
    
  vector_init(v, object_size);

  return v;
}

void vector_free(vector *v)
{
  vector_clear(v);
  free(v);
}

void vector_init(vector *v, size_t object_size)
{
  buffer_init(&v->buffer);
  v->object_size = object_size;
  v->release = NULL;
}

void vector_release(vector *v, void (*release)(void *))
{
  v->release = release;
}

void *vector_deconstruct(vector *v)
{
  void *data;

  (void) vector_shrink_to_fit(v);
  data = buffer_data(&v->buffer);
  free(v);

  return data;
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

int vector_reserve(vector *v, size_t capacity)
{
  return buffer_reserve(&v->buffer, capacity * v->object_size);
}

int vector_shrink_to_fit(vector *v)
{
  return buffer_compact(&v->buffer);
}

/* element access */

void *vector_at(vector *v, size_t position)
{
  return buffer_data(&v->buffer) + (position * v->object_size);
}

void *vector_front(vector *v)
{
  return buffer_data(&v->buffer);
}

void *vector_back(vector *v)
{
  return buffer_data(&v->buffer) + buffer_size(&v->buffer) - v->object_size;
}

void *vector_data(vector *v)
{
  return buffer_data(&v->buffer);
}

/* modifiers */

int vector_push_back(vector *v, void *object)
{
  return buffer_insert(&v->buffer, v->buffer.size, object, v->object_size);
}

void vector_pop_back(vector *v)
{
  size_t size = vector_size(v);

  vector_erase(v, size - 1, size);
}

int vector_insert(vector *v, size_t position, size_t size, void *object)
{
  return buffer_insert(&v->buffer, position * v->object_size, object, size * v->object_size);
}

void vector_erase(vector *v, size_t from, size_t to)
{
  size_t i;

  if (v->release)
    for (i = from; i < to; i ++)
      v->release(vector_at(v, i));
  
  buffer_erase(&v->buffer, from * v->object_size, (to - from) * v->object_size);
}

void vector_clear(vector *v)
{
  vector_erase(v, 0, vector_size(v));
  buffer_clear(&v->buffer);
}

