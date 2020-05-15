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

static __thread reactor_core r_core = {0};

static uint64_t reactor_core_time(void)
{
  struct timespec tv;

  clock_gettime(CLOCK_MONOTONIC, &tv);
  return (uint64_t) tv.tv_sec * 1000000000 + (uint64_t) tv.tv_nsec;
}

void reactor_core_construct(void)
{
  if (!r_core.ref)
    {
      r_core = (reactor_core) {0};
      r_core.now = reactor_core_time();
      r_core.fd = epoll_create1(EPOLL_CLOEXEC);
      reactor_assert_int_not_equal(r_core.fd, -1);
      r_core.now = reactor_core_time();
      vector_construct(&r_core.next, sizeof (reactor_user));
    }

  r_core.ref ++;
}

void reactor_core_destruct(void)
{
  if (!r_core.ref)
    return;

  r_core.ref --;
  if (!r_core.ref)
    (void) close(r_core.fd);
}

void reactor_core_run(void)
{
  reactor_user *user;
  size_t i;

  reactor_assert_int_not_equal(r_core.ref, 0);
  while (r_core.active || vector_size(&r_core.next))
    {
      if (vector_size(&r_core.next))
        {
          user = vector_data(&r_core.next);
          for (i = 0; i < vector_size(&r_core.next); i ++)
            (void) reactor_user_dispatch(&user[i], 0, 0);
          vector_clear(&r_core.next, NULL);
        }

      if (r_core.active)
        {
          reactor_stats_sleep_start();
          r_core.received = epoll_wait(r_core.fd, r_core.events, REACTOR_CORE_MAX_EVENTS, -1);
          reactor_stats_sleep_end(r_core.received);
          reactor_assert_int_not_equal(r_core.received, -1);
          r_core.now = reactor_core_time();
          for (r_core.current = 0; r_core.current < r_core.received; r_core.current ++)
            {
              reactor_stats_event_start();
              (void) reactor_user_dispatch(r_core.events[r_core.current].data.ptr, REACTOR_FD_EVENT, r_core.events[r_core.current].events);
              reactor_stats_event_end();
            }
        }
    }
}

void reactor_core_add(reactor_user *user, int fd, int events)
{
  int e;

  e = epoll_ctl(r_core.fd, EPOLL_CTL_ADD, fd, (struct epoll_event[]){{.events = events, .data.ptr = user}});
  reactor_assert_int_not_equal(e, -1);
  r_core.active ++;
}

void reactor_core_modify(reactor_user *user, int fd, int events)
{
  int e;

  e = epoll_ctl(r_core.fd, EPOLL_CTL_MOD, fd, (struct epoll_event[]){{.events = events, .data.ptr = user}});
  reactor_assert_int_not_equal(e, -1);
}

void reactor_core_deregister(reactor_user *user)
{
  int i;

  for (i = r_core.current; i < r_core.received; i ++)
    if (r_core.events[i].data.ptr == user)
      r_core.events[i].data.ptr = &reactor_user_default;

  r_core.active --;
}

void reactor_core_delete(reactor_user *user, int fd)
{
  int e;

  e = epoll_ctl(r_core.fd, EPOLL_CTL_DEL, fd, NULL);
  reactor_assert_int_not_equal(e, -1);
  reactor_core_deregister(user);
}

reactor_id reactor_core_schedule(reactor_user_callback *callback, void *state)
{
  reactor_user user;

  reactor_user_construct(&user, callback, state);
  vector_push_back(&r_core.next, &user);
  return vector_size(&r_core.next);
}

void reactor_core_cancel(reactor_id id)
{
  if (id > 0)
    ((reactor_user *) vector_data(&r_core.next))[id - 1] = reactor_user_default;
}

uint64_t reactor_core_now(void)
{
  return r_core.now;
}
