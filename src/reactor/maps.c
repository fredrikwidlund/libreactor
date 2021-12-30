#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hash.h"
#include "map.h"
#include "maps.h"

static void set(void *p1, void *p2)
{
  maps_entry *a = p1, *b = p2;

  *a = b ? *b : (maps_entry) {0};
}

static int equal(void *p1, void *p2)
{
  maps_entry *a = p1, *b = p2;

  return b ? strcmp(a->key, b->key) == 0 : a->key == NULL;
}

static size_t hash(void *p)
{
  maps_entry *a = p;

  return hash_string(a->key);
}

/* constructor/destructor */

void maps_construct(maps *maps)
{
  map_construct(&maps->map, sizeof(maps_entry), set);
}

void maps_destruct(maps *maps, maps_release *release)
{
  map_destruct(&maps->map, equal, (map_release *) release);
}

/* capacity */

size_t maps_size(maps *maps)
{
  return map_size(&maps->map);
}

void maps_reserve(maps *maps, size_t size)
{
  map_reserve(&maps->map, size, hash, set, equal);
}

/* element access */

uintptr_t maps_at(maps *maps, char *key)
{
  return ((maps_entry *) map_at(&maps->map, (maps_entry[]) {{.key = key}}, hash, equal))->value;
}

/* modifiers */

void maps_insert(maps *maps, char *key, uintptr_t value, maps_release *release)
{
  map_insert(&maps->map, (maps_entry[]) {{.key = key, .value = value}}, hash, set, equal, (map_release *) release);
}

void maps_erase(maps *maps, char *key, maps_release *release)
{
  map_erase(&maps->map, (maps_entry[]) {{.key = key}}, hash, set, equal, (map_release *) release);
}

void maps_clear(maps *maps, maps_release *release)
{
  map_clear(&maps->map, set, equal, (map_release *) release);
}
