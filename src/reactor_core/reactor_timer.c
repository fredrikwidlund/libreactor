#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_timer.h"

void reactor_timer_init(reactor_timer *timer, reactor_user_call *call, void *state)
{
  (void) reactor_desc_init(&timer->desc, reactor_timer_event, timer);
  (void) reactor_user_init(&timer->user, call, state);
  timer->state = REACTOR_TIMER_CLOSED;
}

int reactor_timer_open(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  int fd, e;

  if (timer->state != REACTOR_TIMER_CLOSED)
    return -1;

  fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1)
    return -1;

  e = reactor_desc_open(&timer->desc, fd);
  if (e == -1)
    {
      (void) close(fd);
      return -1;
    }

  timer->state = REACTOR_TIMER_OPEN;
  return reactor_timer_set(timer, initial, interval);
}

int reactor_timer_set(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  struct timespec t0 = {.tv_sec = initial / 1000000000, .tv_nsec = initial % 1000000000};
  struct timespec tn = {.tv_sec = interval / 1000000000, .tv_nsec = interval % 1000000000};
  struct itimerspec t = {.it_interval = tn, .it_value = t0};

  if (timer->state != REACTOR_TIMER_OPEN)
    return -1;

  return timerfd_settime(reactor_desc_fd(&timer->desc), 0, &t , NULL);
}

void reactor_timer_close(reactor_timer *timer)
{
  if (timer->state != REACTOR_TIMER_OPEN)
    return;

  reactor_timer_set(timer, 0, 0);
  timer->state = REACTOR_TIMER_CLOSING;
  reactor_desc_close(&timer->desc);
}

void reactor_timer_event(void *state, int type, void *data)
{
  reactor_timer *timer;
  uint64_t expirations;
  ssize_t n;

  (void) data;
  timer = state;

  switch (type)
    {
    case REACTOR_DESC_READ:
      if (timer->state != REACTOR_TIMER_OPEN)
        break;
      n = read(reactor_desc_fd(&timer->desc), &expirations, sizeof expirations);
      if (n != sizeof expirations)
        break;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_TIMEOUT, &expirations);
      break;
    case REACTOR_DESC_CLOSE:
      timer->state = REACTOR_TIMER_CLOSED;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_CLOSE, timer);
      break;
    default:
      break;
    }
}
