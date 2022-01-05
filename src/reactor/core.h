#ifndef REACTOR_REACTOR_H_INCLUDED
#define REACTOR_REACTOR_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

enum reactor_event_type
{
  REACTOR_EPOLL_EVENT
};

typedef struct reactor_handler    reactor_handler;
typedef struct reactor_event      reactor_event;
typedef void                      reactor_callback(reactor_event *);
typedef struct reactor            reactor;

struct reactor_handler
{
  reactor_callback   *callback;
  void               *state;
};

struct reactor_event
{
  int                 type;
  void               *state;
  uintptr_t           data;
};

/* reactor_handler */

void     reactor_handler_construct(reactor_handler *, reactor_callback *, void *);
void     reactor_handler_destruct(reactor_handler *);

/* reactor */

uint64_t reactor_now(void);
void     reactor_construct(void);
void     reactor_destruct(void);
void     reactor_add(reactor_handler *, int, uint32_t);
void     reactor_modify(reactor_handler *, int, uint32_t);
void     reactor_delete(reactor_handler *, int);
void     reactor_dispatch(reactor_handler *, int, uintptr_t);
void     reactor_loop_once(void);
void     reactor_loop(void);
void     reactor_abort(void);

#endif /* REACTOR_REACTOR_H_INCLUDED */
