#ifndef MAPS_H_INCLUDED
#define MAPS_H_INCLUDED

#define maps_foreach(m, e) \
  for ((e) = (m)->map.elements; (e) != ((maps_entry *) (m)->map.elements) + (m)->map.elements_capacity; (e) ++) \
    if ((e)->key)

typedef struct maps_entry maps_entry;
typedef struct maps       maps;
typedef void              maps_release(maps_entry *);

struct maps_entry
{
  char      *key;
  uintptr_t  value;
};

struct maps
{
  map        map;
};

/* constructor/destructor */
void      maps_construct(maps *);
void      maps_destruct(maps *, maps_release *);

/* capacity */
size_t    maps_size(maps *);
void      maps_reserve(maps *, size_t);

/* element access */
uintptr_t maps_at(maps *, char *);

/* modifiers */
void      maps_insert(maps *, char *, uintptr_t, maps_release *);
void      maps_erase(maps *, char *, maps_release *);
void      maps_clear(maps *, maps_release *);

#endif /* MAPS_H_INCLUDED */
