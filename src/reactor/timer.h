#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

#include "reactor.h"
#include "descriptor.h"

enum timer_event_type
{
  TIMER_EXPIRATION
};

typedef struct timer timer;
struct timer
{
  reactor_handler handler;
  descriptor      descriptor;
};

void timer_construct(timer *, reactor_callback *, void *);
void timer_destruct(timer *);
void timer_set(timer *, uint64_t, uint64_t);
void timer_clear(timer *);

#endif /* REACTOR_TIMER_H_INCLUDED */
