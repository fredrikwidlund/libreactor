#include <assert.h>
#include <pthread.h>

#include "reactor.h"

static void *async_thread(void *arg)
{
  async *async = arg;

  reactor_dispatch(&async->handler, ASYNC_CALL, 0);
  if (event_is_open(&async->event))
      event_signal(&async->event);
  return NULL;
}

static void async_callback(reactor_event *event)
{
  async *async = event->state;

  assert(event->type == EVENT_SIGNAL);
  async->thread = 0;
  async_cancel(async);
  reactor_dispatch(&async->handler, ASYNC_DONE, 0);
}

void async_construct(async *async, reactor_callback *callback, void *state)
{
  *async = (struct async) {0};
  reactor_handler_construct(&async->handler, callback, state);
  event_construct(&async->event, async_callback, async);
}

void async_destruct(async *async)
{
  async_cancel(async);
  event_destruct(&async->event);
}

void async_call(async *async)
{
  int e;
  pthread_attr_t attr;

  event_open(&async->event);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  e = pthread_create(&async->thread, &attr, async_thread, async);
  assert(e == 0);
}

void async_cancel(async *async)
{
  if (event_is_open(&async->event))
  {
    event_close(&async->event);
    if (async->thread)
    {
      (void) pthread_cancel(async->thread);
      async->thread = 0;
    }
  }
}
