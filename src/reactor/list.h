#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#define list_foreach(l, o)         for ((o) = list_front(l); (o) != list_end(l); (o) = list_next(o))
#define list_foreach_reverse(l, o) for ((o) = list_back(l); (o) != list_end(l); (o) = list_previous(o))

typedef void             list_release(void *);
typedef int              list_compare(void *, void *);
typedef struct list_item list_item;
typedef struct list      list;

struct list
{
  list_item *next;
  list_item *previous;
};

struct list_item
{
  list       list;
  char       object[];
};

/* constructor/destructor */
void  list_construct(list *);
void  list_destruct(list *, list_release *);

/* iterators */
void *list_next(void *);
void *list_previous(void *);

/* capacity */
int   list_empty(list *);

/* object access */
void *list_front(list *);
void *list_back(list *);
void *list_end(list *);

/* modifiers */
void *list_push_front(list *, void *, size_t);
void *list_push_back(list *, void *, size_t);
void *list_insert(void *, void *, size_t);
void  list_splice(void *, void *);
void  list_erase(void *, list_release *);
void  list_clear(list *, list_release *);

/* operations */
void *list_find(list *, list_compare *, void *);

#endif /* LIST_H_INCLUDED */
