#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <poll.h>
#include <signal.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_timer.h"

static void reactor_timer_close_fd(reactor_timer *timer)
{
  if (timer->fd == -1)
    return;
  reactor_core_fd_deregister(timer->fd);
  (void) close(timer->fd);
  timer->fd = -1;
}

static void reactor_timer_hold(reactor_timer *timer)
{
  timer->ref ++;
}

static void reactor_timer_release(reactor_timer *timer)
{
  timer->ref --;
  if (!timer->ref)
    {
      reactor_timer_close_fd(timer);
      timer->state = REACTOR_TIMER_STATE_CLOSED;
      reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_CLOSE, timer);
    }
}

static void reactor_timer_error(reactor_timer *timer)
{
  reactor_timer_close_fd(timer);
  timer->state = REACTOR_TIMER_STATE_ERROR;
  reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_ERROR, NULL);
}

static void reactor_timer_event(void *state, int type, void *data)
{
  reactor_timer *timer = state;
  short revents = ((struct pollfd *) data)->revents;
  uint64_t expirations;
  ssize_t n;

  (void) type;
  if (revents & (POLLERR | POLLNVAL))
    reactor_timer_error(timer);
  else if (revents & POLLIN)
    {
      n = read(timer->fd, &expirations, sizeof expirations);
      if (n == sizeof expirations)
        reactor_user_dispatch(&timer->user, REACTOR_TIMER_EVENT_CALL, &expirations);
      else
        reactor_timer_error(timer);
    }
}

void reactor_timer_open(reactor_timer *timer, reactor_user_callback *callback, void *state,
                             uint64_t initial, uint64_t interval)
{
  timer->ref = 0;
  timer->state = REACTOR_TIMER_STATE_OPEN;
  reactor_user_construct(&timer->user, callback, state);
  reactor_timer_hold(timer);
  timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (timer->fd == -1)
    {
      reactor_timer_error(timer);
      return;
    }
  reactor_core_fd_register(timer->fd, reactor_timer_event, timer, POLLIN);
  reactor_timer_set(timer, initial, interval);
}

void reactor_timer_set(reactor_timer *timer, uint64_t initial, uint64_t interval)
{
  int e;

  e = timerfd_settime(timer->fd, 0, (struct itimerspec[]) {{
        .it_interval = {.tv_sec = interval / 1000000000, .tv_nsec = interval % 1000000000},
        .it_value = {.tv_sec = initial / 1000000000, .tv_nsec = initial % 1000000000}
      }}, NULL);
  if (e == -1)
    reactor_timer_error(timer);
}

void reactor_timer_close(reactor_timer *timer)
{
  if (timer->state & REACTOR_TIMER_STATE_CLOSED)
    return;

  reactor_timer_close_fd(timer);
  reactor_timer_release(timer);
}
