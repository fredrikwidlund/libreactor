#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "list.h"

/* internals */

static list_item *list_object_item(void *object)
{
  return (list_item *) ((uintptr_t) object - offsetof(list_item, object));
}

static list_item *list_item_new(void *object, size_t size)
{
  list_item *item;

  item = calloc(1, sizeof(list_item) + size);
  if (!item)
    abort();

  if (object)
    memcpy(item->object, object, size);

  return item;
}

/* constructor/destructor */

void list_construct(list *l)
{
  l->next = (list_item *) l;
  l->previous = (list_item *) l;
}

void list_destruct(list *l, list_release *release)
{
  list_clear(l, release);
}

/* iterators */

void *list_next(void *object)
{
  return list_object_item(object)->list.next->object;
}

void *list_previous(void *object)
{
  return list_object_item(object)->list.previous->object;
}

/* capacity */

int list_empty(list *l)
{
  return l->next == (list_item *) l;
}

/* element access */

void *list_front(list *l)
{
  return l->next->object;
}

void *list_back(list *l)
{
  return l->previous->object;
}

void *list_end(list *l)
{
  return ((list_item *) l)->object;
}

/* modifiers */

void *list_push_front(list *l, void *object, size_t size)
{
  return list_insert(list_front(l), object, size);
}

void *list_push_back(list *l, void *object, size_t size)
{
  return list_insert(list_end(l), object, size);
}

void *list_insert(void *list_object, void *object, size_t size)
{
  list_item *after, *item;

  after = list_object_item(list_object);

  item = list_item_new(object, size);
  item->list.previous = after->list.previous;
  item->list.next = after;
  item->list.previous->list.next = item;
  item->list.next->list.previous = item;

  return item->object;
}

void list_splice(void *object1, void *object2)
{
  list_item *to, *from;

  if (object1 == object2)
    return;

  to = list_object_item(object1);
  from = list_object_item(object2);

  from->list.previous->list.next = from->list.next;
  from->list.next->list.previous = from->list.previous;

  from->list.previous = to->list.previous;
  from->list.next = to;
  from->list.previous->list.next = from;
  from->list.next->list.previous = from;
}

void list_erase(void *object, list_release *release)
{
  list_item *item = list_object_item(object);

  item->list.previous->list.next = item->list.next;
  item->list.next->list.previous = item->list.previous;

  if (release)
    release(object);

  free(item);
}

void list_clear(list *l, list_release *release)
{
  while (!list_empty(l))
    list_erase(list_front(l), release);
}

/* operations */

void *list_find(list *l, list_compare *compare, void *object)
{
  void *list_object;

  list_foreach(l, list_object) if (compare(object, list_object) == 0) return list_object;

  return NULL;
}
