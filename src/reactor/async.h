#ifndef REACTOR_ASYNC_H_INCLUDED
#define REACTOR_ASYNC_H_INCLUDED

#include "pthread.h"
#include "core.h"
#include "event.h"

enum
{
  ASYNC_CALL,
  ASYNC_DONE
};

typedef struct async async;

struct async
{
  reactor_handler handler;
  pthread_t       thread;
  event           event;
};

void async_construct(async *, reactor_callback *, void *);
void async_destruct(async *);
void async_call(async *);
void async_cancel(async *);

#endif /* REACTOR_ASYNC_H_INCLUDED */
