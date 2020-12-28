#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include <dynamic.h>

#include "timer.h"

static core_status timer_next(core_event *event)
{
  timer *timer = event->state;

  timer->next = 0;
  return core_dispatch(&timer->user, TIMER_ERROR, 0);
}

static void timer_abort(timer *timer)
{
  timer_clear(timer);
  timer->next = core_next(NULL, timer_next, timer);
}

static core_status timer_callback(core_event *event)
{
  timer *timer = event->state;
  uint64_t expirations;
  ssize_t n;
  core_status e;

  if (event->data != EPOLLIN)
    return core_dispatch(&timer->user, TIMER_ERROR, 0);

  while (1)
  {
    n = read(timer->fd, &expirations, sizeof expirations);
    if (n == -1 && errno == EAGAIN)
      return CORE_OK;

    if (n != sizeof expirations)
      return core_dispatch(&timer->user, TIMER_ERROR, 0);

    e = core_dispatch(&timer->user, TIMER_ALARM, expirations);
    if (e != CORE_OK)
      return e;
  }
}

void timer_construct(timer *timer, core_callback *callback, void *state)
{
  *timer = (struct timer) {.user = {.callback = callback, .state = state}, .fd = -1};
}

void timer_destruct(timer *timer)
{
  timer_clear(timer);
  if (timer->next)
  {
    core_cancel(NULL, timer->next);
    timer->next = 0;
  }
}

void timer_set(timer *timer, uint64_t t0, uint64_t ti)
{
  int e;

  if (timer->fd == -1)
  {
    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer->fd == -1)
    {
      timer_abort(timer);
      return;
    }
    core_add(NULL, timer_callback, timer, timer->fd, EPOLLIN | EPOLLET);
  }

  if (t0 == 0)
    t0 = 1;

  e = timerfd_settime(timer->fd, 0, (struct itimerspec[]) {{.it_interval = {.tv_sec = ti / 1000000000, .tv_nsec = ti % 1000000000}, .it_value = {.tv_sec = t0 / 1000000000, .tv_nsec = t0 % 1000000000}}}, NULL);
  if (e == -1)
    timer_abort(timer);
}

void timer_clear(timer *timer)
{
  if (timer->fd >= 0)
  {
    core_delete(NULL, timer->fd);
    (void) close(timer->fd);
    timer->fd = -1;
  }
}
