#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"

typedef struct reactor_core reactor_core;
struct reactor_core
{
  vector       polls;
  vector       users;
  reactor_pool pool;
};

static __thread struct reactor_core core = {0};

static void reactor_core_grow(size_t size)
{
  size_t current;

  current = vector_size(&core.polls);
  if (size > current)
    {
      vector_insert_fill(&core.polls, current, size - current, (struct pollfd[]) {{.fd = -1}});
      vector_insert_fill(&core.users, current, size - current, (reactor_user[]) {{0}});
    }
}

static void reactor_core_shrink(void)
{
  while (vector_size(&core.polls) && (((struct pollfd *) vector_back(&core.polls))->fd == -1))
    {
      vector_pop_back(&core.polls);
      vector_pop_back(&core.users);
    }
}

void reactor_core_construct()
{
  vector_construct(&core.polls, sizeof (struct pollfd));
  vector_construct(&core.users, sizeof (reactor_user));
  reactor_pool_construct(&core.pool);
}

void reactor_core_destruct()
{
  reactor_pool_destruct(&core.pool);
  vector_destruct(&core.polls);
  vector_destruct(&core.users);
}

int reactor_core_run(void)
{
  int e;
  size_t i;
  struct pollfd *pollfd;

  while (vector_size(&core.polls))
    {
      e = poll(vector_data(&core.polls), vector_size(&core.polls), -1);
      if (e == -1)
        return -1;

      for (i = 0; i < vector_size(&core.polls); i ++)
        {
          pollfd = reactor_core_fd_poll(i);
          if (pollfd->revents)
            reactor_user_dispatch(vector_at(&core.users, i), REACTOR_CORE_EVENT_FD, pollfd);
        }
    }

  return 0;
}

void reactor_core_fd_register(int fd, reactor_user_callback *callback, void *state, int events)
{
  reactor_core_grow(fd + 1);
  *(struct pollfd *) reactor_core_fd_poll(fd) = (struct pollfd) {.fd = fd, .events = events};
  *(reactor_user *) reactor_core_fd_user(fd) = (reactor_user) {.callback = callback, .state = state};
}

void reactor_core_fd_deregister(int fd)
{
  *(struct pollfd *) reactor_core_fd_poll(fd) = (struct pollfd) {.fd = -1};
  reactor_core_shrink();
}

void *reactor_core_fd_poll(int fd)
{
  return vector_at(&core.polls, fd);
}

void *reactor_core_fd_user(int fd)
{
  return vector_at(&core.users, fd);
}

void reactor_core_job_register(reactor_user_callback *callback, void *state)
{
  reactor_pool_enqueue(&core.pool, callback, state);
}
