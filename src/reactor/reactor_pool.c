#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <sys/socket.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"

static int reactor_pool_worker_thread(void *arg)
{
  reactor_pool *pool = arg;
  reactor_pool_job *job;
  ssize_t n;

  while (1)
    {
      n = read(pool->queue[1], &job, sizeof job);
      if (n != sizeof job)
        return 1;
      reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_CALL, NULL);
      n = write(pool->queue[1], &job, sizeof job);
      if (n != sizeof job)
        return 1;
    }

  return 0;
}

static void reactor_pool_event(void *state, int type, void *data)
{
  reactor_pool *pool = state;
  reactor_pool_job *job;
  short revents = ((struct pollfd *) data)->revents;
  ssize_t n;

  (void) type;
  if (revents & POLLIN)
    {
      n = read(pool->queue[0], &job, sizeof job);
      if (n != sizeof job)
        return;
      pool->jobs --;
      if (!pool->jobs)
        reactor_core_fd_deregister(pool->queue[0]);
      reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_RETURN, NULL);
      free(job);
    }
}

static void reactor_pool_grow(reactor_pool *pool)
{
  reactor_pool_worker worker;

  worker.stack = malloc(REACTOR_POOL_STACK_SIZE);
  worker.pid = clone(reactor_pool_worker_thread, (char *) worker.stack + REACTOR_POOL_STACK_SIZE, CLONE_VM, pool);
  vector_push_back(&pool->workers, &worker);
}

static void reactor_pool_enqueue(reactor_pool *pool, reactor_pool_job *job)
{
  if (pool->jobs >= vector_size(&pool->workers))
    reactor_pool_grow(pool);
  if (!pool->jobs)
    reactor_core_fd_register(pool->queue[0], reactor_pool_event, pool, POLLIN);
  pool->jobs ++;
  (void) write(pool->queue[0], &job, sizeof job);
}

void reactor_pool_construct(reactor_pool *pool)
{
  pool->jobs = 0;
  vector_construct(&pool->workers, sizeof (reactor_pool_worker));
  (void) socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pool->queue);
  (void) fcntl(pool->queue[0], F_SETFL,O_NONBLOCK);
}

void reactor_pool_create_job(reactor_pool *pool, reactor_user_callback *callback, void *state)
{
  reactor_pool_job *job;

  job = malloc(sizeof *job);
  reactor_user_construct(&job->user, callback, state);
  reactor_pool_enqueue(pool, job);
}
