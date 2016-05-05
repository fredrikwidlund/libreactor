#include <unistd.h>
#include <signal.h>
#include <sys/signalfd.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_signal.h"

void reactor_signal_init(reactor_signal *signal, reactor_user_call *call, void *state)
{
  (void) reactor_desc_init(&signal->desc, reactor_signal_event, signal);
  (void) reactor_user_init(&signal->user, call, state);
  signal->state = REACTOR_SIGNAL_CLOSED;
}

int reactor_signal_open(reactor_signal *signal, sigset_t *mask)
{
  int e, fd;

  if (signal->state != REACTOR_SIGNAL_CLOSED)
    return -1;

  e = sigprocmask(SIG_BLOCK, mask, NULL);
  if (e == -1)
    return -1;

  fd = signalfd(-1, mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (fd == -1)
    return -1;

  e = reactor_desc_open(&signal->desc, fd);
  if (e == -1)
    {
      (void) close(fd);
      return -1;
    }

  signal->state = REACTOR_SIGNAL_OPEN;
  return 0;
}

void reactor_signal_close(reactor_signal *signal)
{
  if (signal->state != REACTOR_SIGNAL_OPEN)
    return;

  signal->state = REACTOR_SIGNAL_CLOSING;
  reactor_desc_close(&signal->desc);
}

void reactor_signal_scheduled_close(reactor_signal *signal)
{
  if (signal->state != REACTOR_SIGNAL_OPEN)
    return;

  signal->state = REACTOR_SIGNAL_CLOSED;
  reactor_desc_scheduled_close(&signal->desc);
}

void reactor_signal_event(void *state, int type, void *data)
{
  reactor_signal *signal;
  struct signalfd_siginfo fdsi;
  ssize_t n;

  (void) data;
  signal = state;

  switch (type)
    {
    case REACTOR_DESC_READ:
      while (signal->state == REACTOR_SIGNAL_OPEN)
        {
          n = read(reactor_desc_fd(&signal->desc), &fdsi, sizeof fdsi);
          if (n != sizeof fdsi)
            break;

          reactor_user_dispatch(&signal->user, REACTOR_SIGNAL_RAISE, &fdsi);
        }
      break;
    case REACTOR_DESC_CLOSE:
      signal->state = REACTOR_SIGNAL_CLOSED;
      reactor_user_dispatch(&signal->user, REACTOR_SIGNAL_CLOSE, signal);
      break;
    default:
      break;
    }
}
