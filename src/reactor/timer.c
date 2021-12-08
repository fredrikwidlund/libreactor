#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "reactor.h"

#include "timer.h"

static void timer_callback(reactor_event *event)
{
  timer *timer = event->state;
  uint64_t expirations, total;
  ssize_t n;

  total = 0;
  while (1)
  {
    n = read(descriptor_fd(&timer->descriptor), &expirations, sizeof expirations);
    if (n == -1)
    {
      assert(errno == EAGAIN);
      break;
    }
    assert(n == sizeof expirations);
    total += expirations;
  }
  assert(total);
  reactor_dispatch(&timer->handler, TIMER_EXPIRATION, total);
}

void timer_construct(timer *timer, reactor_callback *callback, void *state)
{
  reactor_handler_construct(&timer->handler, callback, state);
  descriptor_construct(&timer->descriptor, timer_callback, timer);
}

void timer_destruct(timer *timer)
{
  timer_clear(timer);
}

void timer_set(timer *timer, uint64_t t0, uint64_t ti)
{
  int e, fd;

  if (descriptor_fd(&timer->descriptor) == -1)
  {
    fd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    assert(fd != -1);
    descriptor_open(&timer->descriptor, fd, 0);
  }
  t0 += t0 == 0;
  e = timerfd_settime(descriptor_fd(&timer->descriptor), 0, (struct itimerspec[]) {{
        .it_interval = {.tv_sec = ti / 1000000000, .tv_nsec = ti % 1000000000},
        .it_value = {.tv_sec = t0 / 1000000000, .tv_nsec = t0 % 1000000000}}}, NULL);
  assert(e != -1);
}

void timer_clear(timer *timer)
{
  descriptor_close(&timer->descriptor);
}
