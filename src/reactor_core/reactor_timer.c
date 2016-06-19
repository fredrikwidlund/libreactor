#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_timer.h"

static inline void reactor_timer_close_final(reactor_timer *timer)
{
  if (timer->state == REACTOR_TIMER_CLOSE_WAIT &&
      timer->desc.state == REACTOR_DESC_CLOSED &&
      timer->ref == 0)
    {
      timer->state = REACTOR_TIMER_CLOSED;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_CLOSE, NULL);
    }
}

static inline void reactor_timer_hold(reactor_timer *timer)
{
  timer->ref ++;
}

static inline void reactor_timer_release(reactor_timer *timer)
{
  timer->ref --;
  reactor_timer_close_final(timer);
}

void reactor_timer_init(reactor_timer *timer, reactor_user_callback *callback, void *state)
{
  *timer = (reactor_timer) {.state = REACTOR_TIMER_CLOSED};
  reactor_user_init(&timer->user, callback, state);
  reactor_desc_init(&timer->desc, reactor_timer_event, timer);
}

void reactor_timer_open(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  int fd;

  if (timer->state != REACTOR_TIMER_CLOSED)
    {
      reactor_timer_error(timer);
      return;
    }
  timer->state = REACTOR_TIMER_OPEN;

  fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1)
    {
      reactor_timer_error(timer);
      return;
    }

  reactor_timer_hold(timer);
  reactor_desc_open(&timer->desc, fd, REACTOR_DESC_FLAGS_READ);
  if (timer->state == REACTOR_TIMER_OPEN)
    reactor_timer_set(timer, initial, interval);
  reactor_timer_release(timer);
}

void reactor_timer_set(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  int e;

  e = timerfd_settime(reactor_desc_fd(&timer->desc), 0, (struct itimerspec[]) {{
        .it_interval = {.tv_sec = interval / 1000000000, .tv_nsec = interval % 1000000000},
        .it_value = {.tv_sec = initial / 1000000000, .tv_nsec = initial % 1000000000}
      }}, NULL);
  if (e == -1)
    reactor_timer_error(timer);
}

void reactor_timer_error(reactor_timer *timer)
{
  timer->state = REACTOR_TIMER_INVALID;
  reactor_user_dispatch(&timer->user, REACTOR_TIMER_ERROR, NULL);
}

void reactor_timer_close(reactor_timer *timer)
{
  if (timer->state == REACTOR_TIMER_CLOSED)
    return;
  timer->state = REACTOR_TIMER_CLOSE_WAIT;
  reactor_desc_close(&timer->desc);
  reactor_timer_close_final(timer);
}

void reactor_timer_event(void *state, int type, void *data)
{
  reactor_timer *timer = state;
  uint64_t expirations;
  ssize_t n;

  (void) data;
  switch(type)
    {
    case REACTOR_DESC_READ:
      n = read(reactor_desc_fd(&timer->desc), &expirations, sizeof expirations);
      if (n != sizeof expirations)
        break;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_SIGNAL, &expirations);
      break;
    case REACTOR_DESC_ERROR:
      reactor_timer_error(timer);
      break;
    case REACTOR_DESC_CLOSE:
      reactor_timer_close_final(timer);
      break;
    }
}
