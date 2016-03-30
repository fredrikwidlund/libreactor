#define _GNU_SOURCE

#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"

static __thread reactor_signal_dispatcher_singleton singleton = {.ref = 0};

void reactor_signal_dispatcher_init(reactor_signal_dispatcher *dispatcher, reactor_user_call *call, void *state)
{
  *dispatcher = (reactor_signal_dispatcher) {.fd = -1};
  reactor_user_init(&dispatcher->user, call, state);
}

int reactor_signal_dispatcher_open(reactor_signal_dispatcher *dispatcher)
{
  int e;

  if (!singleton.ref)
    {
      reactor_stream_init(&singleton.stream, reactor_signal_dispatcher_event, &singleton);
      e = pipe(singleton.pipe);
      if (e == -1)
        return -1;

      e = reactor_stream_open(&singleton.stream, singleton.pipe[0]);
      if (e == -1)
        return -1;
    }
  singleton.ref ++;

  dispatcher->fd = singleton.pipe[1];
  return 0;
}

void reactor_signal_dispatcher_close(reactor_signal_dispatcher *dispatcher)
{
  (void) dispatcher;
  if (!singleton.ref)
    return;

  singleton.ref --;
  if (!singleton.ref && (singleton.flags & REACTOR_SIGNAL_DISPATCHER_RELEASE) == 0)
    {
      singleton.flags |= REACTOR_SIGNAL_DISPATCHER_RELEASE;
      reactor_core_schedule(reactor_signal_dispatcher_call, NULL);
    }
}

void reactor_signal_dispatcher_call(void *state, int type, void *data)
{
  (void) state;
  (void) type;
  (void) data;

  singleton.flags &= ~REACTOR_SIGNAL_DISPATCHER_RELEASE;
  if (!singleton.ref && singleton.stream.state == REACTOR_STREAM_OPEN)
    reactor_stream_close(&singleton.stream);
  (void) close(singleton.pipe[0]);
  (void) close(singleton.pipe[1]);
}

void reactor_signal_dispatcher_sigev(reactor_signal_dispatcher *dispatcher, struct sigevent *sigev)
{
  *sigev = (struct sigevent) {0};
  sigev->sigev_notify = SIGEV_THREAD;
  sigev->sigev_notify_function = reactor_signal_dispatcher_notifier;
  sigev->sigev_value.sival_ptr = dispatcher;
}

void reactor_signal_dispatcher_notifier(union sigval sival)
{
  reactor_signal_dispatcher *dispatcher = sival.sival_ptr;

  (void) write(dispatcher->fd, &dispatcher, sizeof &dispatcher);
}

void reactor_signal_dispatcher_event(void *state, int type, void *data)
{
  reactor_stream_data *message;
  reactor_signal_dispatcher *dispatcher;

  (void) state;

  if (type == REACTOR_STREAM_DATA)
    {
      message = data;
      while (message->size >= sizeof dispatcher)
        {
          memcpy(&dispatcher, message->base, sizeof dispatcher);
          reactor_user_dispatch(&dispatcher->user, REACTOR_SIGNAL_DISPATCHER_MESSAGE, dispatcher);
          reactor_stream_data_consume(message, sizeof dispatcher);
        }
    }
}
