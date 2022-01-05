#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>

#include "reactor.h"
#include "core.h"

#define REACTOR_MAX_EVENTS 32

/* reactor_handler */

static void reactor_handler_default_callback(__attribute__((unused)) reactor_event *event) {}
static reactor_handler reactor_handler_default = {reactor_handler_default_callback, NULL};

void reactor_handler_construct(reactor_handler *handler, reactor_callback *callback, void *state)
{
  *handler = callback ? (reactor_handler) {callback, state} : reactor_handler_default;
}

void reactor_handler_destruct(reactor_handler *handler)
{
  *handler = reactor_handler_default;
}

/* reactor */

struct reactor
{
  int                 epoll_fd;
  int                 active;
  size_t              descriptors;
  uint64_t            time;
  struct epoll_event *event;
  struct epoll_event *event_end;
};

static __thread reactor reactor_core = {0};

static void reactor_signal(__attribute__((unused)) int arg)
{
  reactor_abort();
}

uint64_t reactor_now(void)
{
  struct timespec tv;

  if (!reactor_core.time)
  {
    clock_gettime(CLOCK_REALTIME, &tv);
    reactor_core.time = (uint64_t) tv.tv_sec * 1000000000 + (uint64_t) tv.tv_nsec;
  }
  return reactor_core.time;
}

void reactor_construct(void)
{
  reactor_core = (reactor) {.active = 1};
  reactor_core.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  assert(reactor_core.epoll_fd != -1);
  signal(SIGTERM, reactor_signal);
  signal(SIGINT, reactor_signal);
  signal(SIGPIPE, SIG_IGN);
}

void reactor_destruct(void)
{
  int e;

  e = close(reactor_core.epoll_fd);
  assert(e == 0);
}

void reactor_add(reactor_handler *handler, int fd, uint32_t events)
{
  int e;

  reactor_core.descriptors++;
  e = epoll_ctl(reactor_core.epoll_fd, EPOLL_CTL_ADD, fd,
                (struct epoll_event[]) {{.events = events, .data.ptr = handler}});
  assert(e == 0);
}

void reactor_modify(reactor_handler *handler, int fd, uint32_t events)
{
  int e;

  e = epoll_ctl(reactor_core.epoll_fd, EPOLL_CTL_MOD, fd,
                (struct epoll_event[]) {{.events = events, .data.ptr = handler}});
  assert(e == 0);
}

void reactor_delete(reactor_handler *handler, int fd)
{
  struct epoll_event *event;
  int e;

  for (event = reactor_core.event; event < reactor_core.event_end; event++)
    if (event->data.ptr == handler)
      *event = (struct epoll_event) {.data.ptr = &reactor_handler_default};

  e = epoll_ctl(reactor_core.epoll_fd, EPOLL_CTL_DEL, fd, NULL);
  assert(e == 0);
  reactor_core.descriptors--;
}

void reactor_dispatch(reactor_handler *handler, int type, uintptr_t data)
{
  handler->callback((reactor_event[]) {{.type = type, .state = handler->state, .data = data}});
}

void reactor_loop_once(void)
{
  struct epoll_event events[REACTOR_MAX_EVENTS];
  int n;

  reactor_core.time = 0;
  n = epoll_wait(reactor_core.epoll_fd, events, REACTOR_MAX_EVENTS, -1);
  if (n == -1 && errno == EINTR)
    return;
  assert(n >= 0);
  reactor_core.event_end = events + n;
  for (reactor_core.event = events; reactor_core.event < reactor_core.event_end; reactor_core.event++)
    reactor_dispatch(reactor_core.event->data.ptr, REACTOR_EPOLL_EVENT, (uintptr_t) reactor_core.event);
}

void reactor_loop(void)
{
  while (reactor_core.active && reactor_core.descriptors)
    reactor_loop_once();
}

void reactor_abort(void)
{
  reactor_core.active = 0;
}
