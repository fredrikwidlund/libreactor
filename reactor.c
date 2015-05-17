#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>

#include "reactor.h"
#include "reactor_fd.h"

reactor *reactor_new()
{
  reactor *r;
  int e;
  
  r = malloc(sizeof *r);
  if (!r)
    return NULL;
  
  e = reactor_construct(r);
  if (e == -1)
    {
      reactor_delete(r);
      return NULL;
    }
  
  return r;
}

int reactor_construct(reactor *r)
{
  *r = (reactor) {.epollfd = epoll_create1(EPOLL_CLOEXEC)};
  if (r->epollfd == -1)
    return -1;

  return 0;
}

int reactor_destruct(reactor *r)
{
  int e;
  
  if (r->epollfd >= 0)
    {
      e = close(r->epollfd);
      if (e == -1)
	return -1;
    }
  
  r->epollfd = -1;
  return 0;
}

int reactor_delete(reactor *r)
{
  int e;
  
  e = reactor_destruct(r);
  if (e == -1)
    return -1;
  
  free(r);
  return 0;
}

int reactor_run(reactor *r)
{
  struct epoll_event events[REACTOR_EVENTS];
  int i, n;

  while (~r->flags & REACTOR_FLAG_HALT)
    {
      n = epoll_wait(r->epollfd, events, REACTOR_EVENTS, -1);
      if (n == -1)
	return -1;
      
      for (i = 0; i < n; i ++)
	reactor_dispatch(events[i].data.ptr, events[i].events, NULL);
    }

  return 0;
}

void reactor_halt(reactor *r)
{
  r->flags |= REACTOR_FLAG_HALT;
}

void reactor_dispatch(reactor_call *call, int type, void *data)
{
  call->handler((reactor_event[]){{.call = call, .type = type, .data = data}});
}
