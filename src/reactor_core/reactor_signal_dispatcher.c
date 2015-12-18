#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"

static __thread reactor_signal_dispatcher_singleton singleton = {.ref = 0};

void reactor_signal_dispatcher_init(reactor_signal_dispatcher *dispatcher, reactor_user_call *call, void *state)
{
  reactor_user_init(&dispatcher->user, call, state);
  reactor_signal_init(&singleton.signal, reactor_signal_dispatcher_event, &singleton);
}

int reactor_signal_dispatcher_hold(void)
{
  sigset_t mask;
  int e;

  if (singleton.signal.state == REACTOR_SIGNAL_CLOSED)
    {
      sigemptyset(&mask);
      sigaddset(&mask, REACTOR_SIGNAL_DISPATCHER_SIGNO);
      e = reactor_signal_open(&singleton.signal, &mask);
      if (e == -1)
        return -1;
    }

  singleton.ref ++;
  return 0;
}

void reactor_signal_dispatcher_release(void)
{
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
  if (!singleton.ref && singleton.signal.state == REACTOR_SIGNAL_OPEN)
    reactor_signal_close(&singleton.signal);
}

void reactor_signal_dispatcher_sigev(reactor_signal_dispatcher *dispatcher, struct sigevent *sigev)
{
  sigev->sigev_notify = SIGEV_SIGNAL;
  sigev->sigev_signo = REACTOR_SIGNAL_DISPATCHER_SIGNO;
  sigev->sigev_value.sival_ptr = &dispatcher->user;
}

void reactor_signal_dispatcher_event(void *state, int type, void *data)
{
  struct signalfd_siginfo *fdsi = data;
  reactor_signal_dispatcher *dispatcher;

  (void) state;
  if (type == REACTOR_SIGNAL_RAISE && singleton.ref)
    {
      dispatcher = (reactor_signal_dispatcher *) fdsi->ssi_ptr;
      if (fdsi->ssi_pid == (uint32_t) getpid() && fdsi->ssi_uid == getuid())
        reactor_user_dispatch(&dispatcher->user, REACTOR_SIGNAL_DISPATCHER_MESSAGE, fdsi);
    }
}
