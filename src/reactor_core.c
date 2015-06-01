#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/epoll.h>

#include "buffer.h"
#include "vector.h"
#include "reactor_core.h"
#include "reactor_fd.h"

reactor *reactor_new()
{
  reactor *r;
  int e;
  
  r = malloc(sizeof *r);
  if (!r)
    return NULL;
  
  e = reactor_init(r);
  r->flags |= REACTOR_FLAG_ALLOCATED;
  if (e == -1)
    {
      reactor_delete(r);
      return NULL;
    }
  
  return r;
}

int reactor_init(reactor *r)
{ 
  *r = (reactor) {0};

  r->epollfd = epoll_create1(EPOLL_CLOEXEC);
  if (r->epollfd == -1)
    return -1;

  r->schedule = vector_new(sizeof(reactor_user *));
  if (!r->schedule)
    return -1;

  r->flags |= REACTOR_FLAG_ACTIVE;
  return 0;
}

int reactor_delete(reactor *r)
{
  int e;

  if (r->flags & REACTOR_FLAG_EVENT)
    {
      r->flags &= ~(REACTOR_FLAG_ACTIVE | REACTOR_FLAG_DELETE);
      return 0;
    }

  if (r->epollfd >= 0)
    {
      e = close(r->epollfd);
      if (e == -1)
	return -1;
    }

  if (r->schedule)
    vector_free(r->schedule);
  
  if (r->flags & REACTOR_FLAG_ALLOCATED)
    free(r);
  
  return 0;
}

int reactor_run(reactor *r)
{
  struct epoll_event events[REACTOR_MAX_EVENTS];
  int i, n;

  while (r->flags & REACTOR_FLAG_ACTIVE)
    {
      n = epoll_wait(r->epollfd, events, REACTOR_MAX_EVENTS, -1);
      if (n == -1)
	return -1;

      r->flags |= REACTOR_FLAG_EVENT;
      for (i = 0; i < n && r->flags & REACTOR_FLAG_ACTIVE; i ++)
	reactor_dispatch(r, events[i].data.ptr, REACTOR_EVENT, &events[i].events);
      r->flags &= ~REACTOR_FLAG_EVENT;

      while (!vector_empty(r->schedule) && r->flags & REACTOR_FLAG_ACTIVE)
	{
	  reactor_dispatch(r, *(reactor_user **) vector_back(r->schedule), REACTOR_SCHEDULED_CALL, NULL);
	  vector_pop_back(r->schedule);
	}
    }

  if (r->flags & REACTOR_FLAG_DELETE)
    return reactor_delete(r);
  
  return 0;
}

void reactor_halt(reactor *r)
{
  r->flags &= ~REACTOR_FLAG_ACTIVE;
}

void reactor_dispatch(void *source, reactor_user *user, int type, void *data)
{
  user->handler(user->state, (reactor_event[]) {{.source = source, .type = type, .data = data}});
}

int reactor_schedule(reactor *r, reactor_user *user)
{
  return vector_push_back(r->schedule, &user);
}
