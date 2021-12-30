#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "map.h"

static size_t map_roundup(size_t s)
{
  s--;
  s |= s >> 1;
  s |= s >> 2;
  s |= s >> 4;
  s |= s >> 8;
  s |= s >> 16;
  s |= s >> 32;
  s++;

  return s;
}

static void *map_element(map *m, size_t position)
{
  return (char *) m->elements + (position * m->element_size);
}

static void map_release_all(map *m, map_equal *equal, map_release *release)
{
  size_t i;

  if (release)
    for (i = 0; i < m->elements_capacity; i++)
      if (!equal(map_element(m, i), NULL))
        release(map_element(m, i));
}

static void map_rehash(map *m, size_t size, map_hash *hash, map_set *set, map_equal *equal)
{
  map new;
  size_t i;

  size = map_roundup(size);
  new = *m;
  new.elements_count = 0;
  new.elements_capacity = size;
  new.elements = malloc(new.elements_capacity *new.element_size);
  if (!new.elements)
    abort();

  for (i = 0; i < new.elements_capacity; i++)
    set(map_element(&new, i), NULL);

  if (m->elements)
  {
    for (i = 0; i < m->elements_capacity; i++)
      if (!equal(map_element(m, i), NULL))
        map_insert(&new, map_element(m, i), hash, set, equal, NULL);
    free(m->elements);
  }

  *m = new;
}

/* constructor/destructor */

void map_construct(map *m, size_t element_size, map_set *set)
{
  m->elements = NULL;
  m->elements_count = 0;
  m->elements_capacity = 0;
  m->element_size = element_size;
  map_rehash(m, MAP_ELEMENTS_CAPACITY_MIN, NULL, set, NULL);
}

void map_destruct(map *m, map_equal *equal, map_release *release)
{
  map_release_all(m, equal, release);
  free(m->elements);
}

/* capacity */

size_t map_size(map *m)
{
  return m->elements_count;
}

void map_reserve(map *m, size_t size, map_hash *hash, map_set *set, map_equal *equal)
{
  size *= 2;
  if (size > m->elements_capacity)
    map_rehash(m, size, hash, set, equal);
}

/* element access */

void *map_at(map *m, void *element, map_hash *hash, map_equal *equal)
{
  size_t i;
  void *test;

  i = hash(element);
  while (1)
  {
    i &= m->elements_capacity - 1;
    test = map_element(m, i);
    if (equal(test, NULL) || equal(test, element))
      return test;
    i++;
  }
}

/* modifiers */

void map_insert(map *m, void *element, map_hash *hash, map_set *set, map_equal *equal, map_release *release)
{
  void *test;

  map_reserve(m, m->elements_count + 1, hash, set, equal);
  test = map_at(m, element, hash, equal);
  if (equal(test, NULL))
  {
    set(test, element);
    m->elements_count++;
  }
  else if (release)
    release(element);
}

void map_erase(map *m, void *element, map_hash *hash, map_set *set, map_equal *equal, map_release *release)
{
  void *test;
  size_t i, j, k;

  i = hash(element);
  while (1)
  {
    i &= m->elements_capacity - 1;
    test = map_element(m, i);
    if (equal(test, NULL))
      return;
    if (equal(test, element))
      break;
    i++;
  }

  if (release)
    release(test);
  m->elements_count--;

  j = i;
  while (1)
  {
    j = (j + 1) & (m->elements_capacity - 1);
    if (equal(map_element(m, j), NULL))
      break;

    k = hash(map_element(m, j)) & (m->elements_capacity - 1);
    if ((i < j && (k <= i || k > j)) ||
        (i > j && (k <= i && k > j)))
    {
      set(map_element(m, i), map_element(m, j));
      i = j;
    }
  }

  set(map_element(m, i), NULL);
}

void map_clear(map *m, map_set *set, map_equal *equal, map_release *release)
{
  map_release_all(m, equal, release);
  free(m->elements);
  m->elements = NULL;
  m->elements_count = 0;
  m->elements_capacity = 0;
  map_rehash(m, MAP_ELEMENTS_CAPACITY_MIN, NULL, set, NULL);
}
