#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

typedef struct reactor_core reactor_core;
struct reactor_core
{
  int                ref;
  int                fd;
  int                active;
  int                received;
  int                current;
  uint64_t           now;
  struct epoll_event events[REACTOR_CORE_MAX_EVENTS];
  vector             next;
};

static __thread reactor_core core = {0};

static uint64_t reactor_core_time(void)
{
  struct timespec tv;

  clock_gettime(CLOCK_MONOTONIC, &tv);
  return (uint64_t) tv.tv_sec * 1000000000 + (uint64_t) tv.tv_nsec;
}

void reactor_core_construct(void)
{
  if (!core.ref)
    {
      core = (reactor_core) {0};
      core.now = reactor_core_time();
      core.fd = epoll_create1(EPOLL_CLOEXEC);
      reactor_assert_int_not_equal(core.fd, -1);
      core.now = reactor_core_time();
      vector_construct(&core.next, sizeof (reactor_user));
    }

  core.ref ++;
}

void reactor_core_destruct(void)
{
  if (!core.ref)
    return;

  core.ref --;
  if (!core.ref)
    (void) close(core.fd);
}

void reactor_core_run(void)
{
  reactor_user *user;
  size_t i;

  reactor_assert_int_not_equal(core.ref, 0);
  while (core.active || vector_size(&core.next))
    {
      if (vector_size(&core.next))
        {
          user = vector_data(&core.next);
          for (i = 0; i < vector_size(&core.next); i ++)
            (void) reactor_user_dispatch(&user[i], 0, 0);
          vector_clear(&core.next, NULL);
        }

      if (core.active)
        {
          reactor_stats_sleep_start();
          core.received = epoll_wait(core.fd, core.events, REACTOR_CORE_MAX_EVENTS, -1);
          reactor_stats_sleep_end(core.received);
          reactor_assert_int_not_equal(core.received, -1);
          core.now = reactor_core_time();
          for (core.current = 0; core.current < core.received; core.current ++)
            {
              reactor_stats_event_start();
              (void) reactor_user_dispatch(core.events[core.current].data.ptr, REACTOR_FD_EVENT, core.events[core.current].events);
              reactor_stats_event_end();
            }
        }
    }
}

void reactor_core_add(reactor_user *user, int fd, int events)
{
  int e;

  e = epoll_ctl(core.fd, EPOLL_CTL_ADD, fd, (struct epoll_event[]){{.events = events, .data.ptr = user}});
  reactor_assert_int_not_equal(e, -1);
  core.active ++;
}

void reactor_core_modify(reactor_user *user, int fd, int events)
{
  int e;

  e = epoll_ctl(core.fd, EPOLL_CTL_MOD, fd, (struct epoll_event[]){{.events = events, .data.ptr = user}});
  reactor_assert_int_not_equal(e, -1);
}

void reactor_core_deregister(reactor_user *user)
{
  int i;

  for (i = core.current; i < core.received; i ++)
    if (core.events[i].data.ptr == user)
      core.events[i].data.ptr = &reactor_user_default;

  core.active --;
}

void reactor_core_delete(reactor_user *user, int fd)
{
  int e;

  e = epoll_ctl(core.fd, EPOLL_CTL_DEL, fd, NULL);
  reactor_assert_int_not_equal(e, -1);
  reactor_core_deregister(user);
}

reactor_id reactor_core_schedule(reactor_user_callback *callback, void *state)
{
  reactor_user user;

  reactor_user_construct(&user, callback, state);
  vector_push_back(&core.next, &user);
  return vector_size(&core.next);
}

void reactor_core_cancel(reactor_id id)
{
  if (id > 0)
    ((reactor_user *) vector_data(&core.next))[id - 1] = reactor_user_default;
}

uint64_t reactor_core_now(void)
{
  return core.now;
}
