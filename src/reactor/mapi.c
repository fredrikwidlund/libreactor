#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hash.h"
#include "map.h"
#include "mapi.h"

static void set(void *p1, void *p2)
{
  mapi_entry *a = p1, *b = p2;

  *a = b ? *b : (mapi_entry) {0};
}

static int equal(void *p1, void *p2)
{
  mapi_entry *a = p1, *b = p2;

  return b ? a->key == b->key : a->key == 0;
}

static size_t hash(void *p)
{
  mapi_entry *a = p;

  return hash_uint64(a->key);
}

/* constructor/destructor */

void mapi_construct(mapi *mapi)
{
  map_construct(&mapi->map, sizeof(mapi_entry), set);
}

void mapi_destruct(mapi *mapi, mapi_release *release)
{
  map_destruct(&mapi->map, equal, (map_release *) release);
}

/* capacity */

size_t mapi_size(mapi *mapi)
{
  return map_size(&mapi->map);
}

void mapi_reserve(mapi *mapi, size_t size)
{
  map_reserve(&mapi->map, size, hash, set, equal);
}

/* element access */

uintptr_t mapi_at(mapi *mapi, uintptr_t key)
{
  return ((mapi_entry *) map_at(&mapi->map, (mapi_entry[]) {{.key = key}}, hash, equal))->value;
}

/* modifiers */

void mapi_insert(mapi *mapi, uintptr_t key, uintptr_t value, mapi_release *release)
{
  map_insert(&mapi->map, (mapi_entry[]) {{.key = key, .value = value}}, hash, set, equal, (map_release *) release);
}

void mapi_erase(mapi *mapi, uintptr_t key, mapi_release *release)
{
  map_erase(&mapi->map, (mapi_entry[]) {{.key = key}}, hash, set, equal, (map_release *) release);
}

void mapi_clear(mapi *mapi, mapi_release *release)
{
  map_clear(&mapi->map, set, equal, (map_release *) release);
}
