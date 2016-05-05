#include <unistd.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"

static __thread reactor_core core = {.fd = -1};

int reactor_core_construct(void)
{
  if (core.fd != -1)
    return 0;

  core.fd = epoll_create1(EPOLL_CLOEXEC);
  if (core.fd == -1)
    return -1;

  core.ref = 0;
  vector_init(&core.calls, sizeof(reactor_user));

  return 0;
}

int reactor_core_mask_to_epoll(int events)
{
  return
    EPOLLET |
    (events & REACTOR_DESC_READ ? EPOLLIN : 0) |
    (events & REACTOR_DESC_WRITE ? EPOLLOUT : 0);
}

int reactor_core_mask_from_epoll(int events)
{
  return
    (events & EPOLLIN ? REACTOR_DESC_READ : 0) |
    (events & EPOLLOUT ? REACTOR_DESC_WRITE : 0) |
    (events & EPOLLERR ? REACTOR_DESC_ERROR : 0);
}

int reactor_core_add(int fd, int events, reactor_desc *desc)
{
  uint32_t epoll_events;
  int e;

  epoll_events = reactor_core_mask_to_epoll(events);
  e = epoll_ctl(core.fd, EPOLL_CTL_ADD, fd, (struct epoll_event[]){{.events = epoll_events, .data.ptr = desc}});
  if (e == -1)
    return -1;

  core.ref ++;
  return 0;
}

int reactor_core_mod(int fd, int events, reactor_desc *desc)
{
  uint32_t epoll_events;

  epoll_events = reactor_core_mask_to_epoll(events);
  return epoll_ctl(core.fd, EPOLL_CTL_MOD, fd, (struct epoll_event[]){{.events = epoll_events, .data.ptr = desc}});
}

int reactor_core_del(int fd)
{
  core.ref --;
  return close(fd);
}

int reactor_core_run(void)
{
  struct epoll_event events[REACTOR_CORE_QUEUE_SIZE];
  ssize_t n, i;

  while (core.ref)
    {
      n = epoll_wait(core.fd, events, REACTOR_CORE_QUEUE_SIZE, -1);
      if (n == -1)
        return -1;

      for (i = 0; i < n; i ++)
        reactor_desc_event(events[i].data.ptr, reactor_core_mask_from_epoll(events[i].events));

      for (i = 0; i < (int) vector_size(&core.calls); i ++)
        reactor_user_dispatch((reactor_user *) vector_at(&core.calls, i), REACTOR_CORE_CALL, NULL);
      vector_erase(&core.calls, 0, vector_size(&core.calls));
    }

  return 0;
}

void reactor_core_destruct(void)
{
  (void) close(core.fd);
  vector_clear(&core.calls);
  core.fd = -1;
}

void reactor_core_schedule(reactor_user_call *call, void *state)
{
  (void) vector_push_back(&core.calls, (reactor_user[]){{.call = call, .state = state}});
}
