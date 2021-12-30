#ifndef MAP_H_INCLUDED
#define MAP_H_INCLUDED

#ifndef MAP_ELEMENTS_CAPACITY_MIN
#define MAP_ELEMENTS_CAPACITY_MIN 16
#endif /* MAP_ELEMENTS_CAPACITY_MIN */

typedef size_t           map_hash(void *);
typedef int              map_equal(void *, void *);
typedef void             map_set(void *, void *);
typedef void             map_release(void *);
typedef struct map       map;

struct map
{
  void   *elements;
  size_t  elements_count;
  size_t  elements_capacity;
  size_t  element_size;
};

/* constructor/destructor */
void    map_construct(map *, size_t, map_set *);
void    map_destruct(map *, map_equal *, map_release *);

/* capacity */
size_t  map_size(map *);
void    map_reserve(map *, size_t, map_hash *, map_set *, map_equal *);

/* element access */
void   *map_at(map *, void *, map_hash *, map_equal *);

/* modifiers */
void    map_insert(map *, void *, map_hash *, map_set *, map_equal *, map_release *);
void    map_erase(map *, void *,  map_hash *, map_set *, map_equal *, map_release *);
void    map_clear(map *, map_set *, map_equal *, map_release *);

#endif /* MAP_H_INCLUDED */
