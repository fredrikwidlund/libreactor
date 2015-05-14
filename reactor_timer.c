#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "reactor.h"

#include "reactor_timer.h"

reactor_timer *reactor_timer_new(reactor *r, reactor_handler *h, uint64_t interval, void *user)
{
  reactor_timer *t;
  int e;
  
  t = malloc(sizeof *t);
  e = reactor_timer_construct(r, t, h, interval, user);
  if (e == -1)
    {
      (void) reactor_timer_destruct(t);
      return NULL;
    }
  
  return t;
}

int reactor_timer_construct(reactor *r, reactor_timer *t, reactor_handler *h, uint64_t interval, void *data)
{
  int e, fd;

  fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  if (fd == -1)
    return -1;

  *t = (reactor_timer) {.user = {.handler = h, .data = data},
			.reactor = r, .descriptor = fd,
			.ev = {.events = EPOLLIN | EPOLLET, .data.ptr = &t->main},
			.main = {.handler = reactor_timer_handler, .data = t}};
  
  e = reactor_timer_interval(t, interval);
  if (e == -1)
    {
      (void) reactor_timer_destruct(t);
      return -1;
    }

  return epoll_ctl(r->epollfd, EPOLL_CTL_ADD, fd, &t->ev);
}

int reactor_timer_destruct(reactor_timer *t)
{
  int e;

  if (t->descriptor >= 0)
    {
      e = epoll_ctl(t->reactor->epollfd, EPOLL_CTL_DEL, t->descriptor, &t->ev);
      if (e == -1)
	return -1;
      
      e = close(t->descriptor);
      if (e == -1)
	return -1;
      
      t->descriptor = -1;
    }
  
  return 0;
}

int reactor_timer_delete(reactor_timer *t)
{
  int e;
  
  e = reactor_timer_destruct(t);
  if (e == -1)
    return -1;
  
  free(t);
  return 0;
}

int reactor_timer_interval(reactor_timer *t, uint64_t interval)
{
  time_t sec = interval / 1000000000;
  long nsec = interval % 1000000000;  
  return timerfd_settime(t->descriptor, 0, (struct itimerspec[]){{{sec, nsec}, {sec, nsec}}}, NULL);
}

void reactor_timer_handler(reactor_event *e)
{
  reactor_timer *d = e->call->data;
  uint64_t expirations;
  ssize_t n;

  n = read(d->descriptor, &expirations, sizeof expirations);
  if (n == sizeof expirations)
    reactor_dispatch(&d->user, REACTOR_TIMER_TIMEOUT, &expirations);  
}
