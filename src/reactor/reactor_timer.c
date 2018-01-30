#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/param.h>
#include <netdb.h>
#include <errno.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_descriptor.h"
#include "reactor_timer.h"

static int reactor_timer_error(reactor_timer *timer)
{
  return reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_ERROR, NULL);
}

static int reactor_timer_event(void *state, int type, void *data)
{
  reactor_timer *timer = state;
  uint64_t expirations;
  ssize_t n;

  (void) type;
  (void) data;
  n = read(reactor_descriptor_fd(&timer->descriptor), &expirations, sizeof expirations);
  if (n != sizeof expirations)
    return reactor_timer_error(timer);
  else
    return reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_CALL, &expirations);
}

int reactor_timer_open(reactor_timer *timer, reactor_user_callback *callback, void *state, uint64_t t0, uint64_t ti)
{
  int e, fd;

  reactor_user_construct(&timer->user, callback, state);
  fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1)
    return REACTOR_ERROR;

  e = reactor_descriptor_open(&timer->descriptor, reactor_timer_event, timer, fd, EPOLLIN | EPOLLET);
  if (e == REACTOR_ABORT)
    return REACTOR_ERROR;

  return reactor_timer_set(timer, t0, ti);
}

int reactor_timer_set(reactor_timer *timer, uint64_t t0, uint64_t ti)
{
  int e;

  e = timerfd_settime(reactor_descriptor_fd(&timer->descriptor), 0, (struct itimerspec[]) {{
        .it_interval = {.tv_sec = ti / 1000000000, .tv_nsec = ti % 1000000000},
          .it_value = {.tv_sec = t0 / 1000000000, .tv_nsec = t0 % 1000000000}
      }}, NULL);
  return e == -1 ? REACTOR_ERROR : REACTOR_OK;
}

void reactor_timer_close(reactor_timer *timer)
{
  reactor_descriptor_close(&timer->descriptor);
}
