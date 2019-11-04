#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_timer_error(reactor_timer *timer, char *description)
{
  return reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_ERROR, (uintptr_t) description);
}

static reactor_status reactor_timer_handler(reactor_event *event)
{
  reactor_timer *timer = event->state;
  uint64_t expirations;
  ssize_t n;
  int e;

  if (event->data != EPOLLIN)
    return reactor_timer_error(timer, "invalid event data");

  while (1)
    {
      n = read(reactor_fd_fileno(&timer->fd), &expirations, sizeof expirations);
      if (n == -1 && errno == EAGAIN)
        return REACTOR_OK;

      if (n != sizeof expirations)
        return reactor_timer_error(timer, "invalid read size");

      e = reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_ALARM, expirations);
      if (e)
        return REACTOR_ABORT;
    }
}

void reactor_timer_construct(reactor_timer *timer, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&timer->user, callback, state);
  reactor_fd_construct(&timer->fd, reactor_timer_handler, timer);
}

void reactor_timer_destruct(reactor_timer *timer)
{
  reactor_timer_clear(timer);
}

void reactor_timer_set(reactor_timer *timer, uint64_t t0, uint64_t ti)
{
  int e, fileno;

  if (!reactor_fd_active(&timer->fd))
    {
      fileno = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
      reactor_assert_int_not_equal(fileno, -1);
      reactor_fd_open(&timer->fd, fileno, EPOLLIN | EPOLLET);
    }

  if (t0 == 0)
    t0 = 1;

  e = timerfd_settime(reactor_fd_fileno(&timer->fd), 0, (struct itimerspec[])
                      {{
                        .it_interval = {.tv_sec = ti / 1000000000, .tv_nsec = ti % 1000000000},
                        .it_value = {.tv_sec = t0 / 1000000000, .tv_nsec = t0 % 1000000000}
                      }}, NULL);
  reactor_assert_int_not_equal(e, -1);
}

void reactor_timer_clear(reactor_timer *timer)
{
  if (reactor_fd_active(&timer->fd))
    reactor_fd_close(&timer->fd);
}
