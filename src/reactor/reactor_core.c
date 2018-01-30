#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_descriptor.h"
#include "reactor_core.h"
#include "reactor_pool.h"

struct reactor_core
{
  int                 fd;
  int                 events_active;
  int                 events_current;
  int                 events_received;
  struct epoll_event  events[REACTOR_CORE_MAX_EVENTS];
  reactor_pool        pool;
};

static __thread struct reactor_core core = {.fd = -1};

int reactor_core_construct(void)
{
  int e;

  core = (struct reactor_core) {0};
  core.fd = epoll_create1(EPOLL_CLOEXEC);
  if (core.fd == -1)
    return REACTOR_ERROR;

  e = reactor_pool_construct(&core.pool);
  if (e == -1)
    {
      (void) close(core.fd);
      core.fd = -1;
      return REACTOR_ERROR;
    }

  return REACTOR_OK;
}

void reactor_core_destruct(void)
{
  if (core.fd != -1)
    {
      close(core.fd);
      core.fd = -1;
      reactor_pool_destruct(&core.pool);
    }
}

int reactor_core_run(void)
{
  sigset_t set;

  sigemptyset(&set);
  while (core.events_active)
    {
      core.events_received = epoll_pwait(core.fd, core.events, REACTOR_CORE_MAX_EVENTS, -1, &set);
      if (core.events_received == -1)
        return REACTOR_ERROR;
      for (core.events_current = 0; core.events_current < core.events_received; core.events_current ++)
        if (core.events[core.events_current].events)
          (void) reactor_descriptor_event(core.events[core.events_current].data.ptr, core.events[core.events_current].events);
    }

  return REACTOR_OK;
}

int reactor_core_add(reactor_descriptor *descriptor, int events)
{
  int e;

  (void) fcntl(reactor_descriptor_fd(descriptor), F_SETFL, O_NONBLOCK);
  e = epoll_ctl(core.fd, EPOLL_CTL_ADD, reactor_descriptor_fd(descriptor),
                (struct epoll_event[]){{.events = events, .data.ptr = descriptor}});
  if (e == -1)
    return REACTOR_ERROR;

  core.events_active ++;
  return REACTOR_OK;
}

void reactor_core_delete(reactor_descriptor *descriptor)
{
  (void) epoll_ctl(core.fd, EPOLL_CTL_DEL, reactor_descriptor_fd(descriptor), NULL);
  reactor_core_flush(descriptor);
}

void reactor_core_modify(reactor_descriptor *descriptor, int events)
{
  (void) epoll_ctl(core.fd, EPOLL_CTL_MOD, reactor_descriptor_fd(descriptor),
                   (struct epoll_event[]){{.events = events, .data.ptr = descriptor}});
}

void reactor_core_flush(reactor_descriptor *descriptor)
{
  ssize_t i;

  core.events_active --;
  for (i = core.events_current; i < core.events_received; i ++)
    if (core.events[i].data.ptr == descriptor)
      core.events[i].events = 0;
}

void reactor_core_enqueue(reactor_user_callback *callback, void *state)
{
  reactor_pool_enqueue(&core.pool, callback, state);
}
