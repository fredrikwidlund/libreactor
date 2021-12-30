#ifndef MAPI_H_INCLUDED
#define MAPI_H_INCLUDED

#define mapi_foreach(m, e) \
  for ((e) = (m)->map.elements; (e) != ((mapi_entry *) (m)->map.elements) + (m)->map.elements_capacity; (e) ++) \
    if ((e)->key)

typedef struct mapi_entry mapi_entry;
typedef struct mapi       mapi;
typedef void              mapi_release(mapi_entry *);

struct mapi_entry
{
  uintptr_t key;
  uintptr_t value;
};

struct mapi
{
  map       map;
};

/* constructor/destructor */
void      mapi_construct(mapi *);
void      mapi_destruct(mapi *, mapi_release *);

/* capacity */
size_t    mapi_size(mapi *);
void      mapi_reserve(mapi *, size_t);

/* element access */
uintptr_t mapi_at(mapi *, uintptr_t);

/* modifiers */
void      mapi_insert(mapi *, uintptr_t, uintptr_t, mapi_release *);
void      mapi_erase(mapi *, uintptr_t, mapi_release *);
void      mapi_clear(mapi *, mapi_release *);

#endif /* MAPI_H_INCLUDED */
