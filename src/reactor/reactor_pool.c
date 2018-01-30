#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <sched.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_descriptor.h"
#include "reactor_core.h"
#include "reactor_pool.h"

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

static void reactor_pool_grow(reactor_pool *pool)
{
  reactor_pool_worker *worker;

  if (pool->workers_count >= pool->workers_max)
    return;

  worker = malloc(sizeof *worker);
  worker->stack = malloc(REACTOR_POOL_STACK_SIZE);
  worker->pid = clone(reactor_pool_worker_thread, (char *) worker->stack + REACTOR_POOL_STACK_SIZE,
                      CLONE_VM | CLONE_FS | CLONE_SIGHAND | CLONE_PARENT,
                      pool);
  list_push_back(&pool->workers, &worker, sizeof worker);
  pool->workers_count ++;
}

static void reactor_pool_dequeue(reactor_pool *pool)
{
  reactor_pool_job *job;
  ssize_t n;

  n = read(pool->queue[0], &job, sizeof job);
  if (n != sizeof job)
    {
      reactor_descriptor_clear(&pool->descriptor);
      return;
    }

  (void) reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_RETURN, NULL);
  free(job);
  pool->jobs_count --;
  if (!pool->jobs_count)
    reactor_descriptor_clear(&pool->descriptor);
}

static void reactor_pool_flush(reactor_pool *pool)
{
  reactor_pool_job **i;
  ssize_t n;

  while (!list_empty(&pool->jobs))
    {
      i = list_front(&pool->jobs);
      n = write(pool->queue[0], i, sizeof *i);
      if (n != sizeof *i)
        break;
      list_erase(i, NULL);
    }
}

#ifndef GCOV_BUILD
static
#endif /* GCOV_BUILD */
int reactor_pool_event(void *state, int type, void *data)
{
  reactor_pool *pool = state;
  int flags = *(int *) data;

  (void) type;
  if (flags & POLLIN)
    reactor_pool_dequeue(pool);
  if (flags & POLLOUT)
    reactor_pool_flush(pool);

  return REACTOR_OK;
}

int reactor_pool_construct(reactor_pool *pool)
{
  int e;

  *pool = (reactor_pool) {0};
  list_construct(&pool->jobs);
  list_construct(&pool->workers);
  pool->workers_max = REACTOR_POOL_WORKERS_MAX;

  e = socketpair(PF_UNIX, SOCK_DGRAM, 0, pool->queue);
  if (e == -1)
    return REACTOR_ERROR;
  return REACTOR_OK;
}

void reactor_pool_destruct(reactor_pool *pool)
{
  reactor_pool_worker **w, *worker;
  reactor_pool_job **j, *job;

  while (!list_empty(&pool->workers))
    {
      w = list_front(&pool->workers);
      worker = *w;
      list_erase(w, NULL);
      pool->workers_count --;
      kill(worker->pid, SIGTERM);
      waitpid(worker->pid, NULL, 0);
      free(worker->stack);
      free(worker);
    }

  while (!list_empty(&pool->jobs))
    {
      j = list_front(&pool->jobs);
      job = *j;
      list_erase(j, NULL);
      pool->jobs_count --;
      free(job);
    }

  if (pool->queue[0] >= 0)
    {
      reactor_descriptor_clear(&pool->descriptor);
      (void) close(pool->queue[0]);
      pool->queue[0] = -1;
    }

  if (pool->queue[1] >= 0)
    {
      (void) close(pool->queue[1]);
      pool->queue[1] = -1;
    }
}

void reactor_pool_limits(reactor_pool *pool, size_t min, size_t max)
{
  pool->workers_min = min;
  pool->workers_max = max;
  while (pool->workers_count < pool->workers_min)
    reactor_pool_grow(pool);
}

void reactor_pool_enqueue(reactor_pool *pool, reactor_user_callback *callback, void *state)
{
  reactor_pool_job *job;
  ssize_t n;

  job = malloc(sizeof *job);
  if (!job)
    abort();
  reactor_user_construct(&job->user, callback, state);
  if (pool->jobs_count >= pool->workers_count)
    reactor_pool_grow(pool);
  if (!pool->jobs_count)
    reactor_descriptor_open(&pool->descriptor, reactor_pool_event, pool, pool->queue[0], POLLIN);

  pool->jobs_count ++;

  if (list_empty(&pool->jobs))
    {
      n = write(pool->queue[0], &job, sizeof job);
      if (n == sizeof job)
        return;
      reactor_descriptor_set(&pool->descriptor, POLLIN | POLLOUT);
    }

  list_push_back(&pool->jobs, &job, sizeof job);
}

