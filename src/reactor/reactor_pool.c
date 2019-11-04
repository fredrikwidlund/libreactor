#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

typedef struct reactor_pool_job     reactor_pool_job;
typedef struct reactor_pool_worker  reactor_pool_worker;
typedef struct reactor_pool         reactor_pool;

struct reactor_pool_job
{
  reactor_user         user;
};

struct reactor_pool_worker
{
  pthread_t           thread;
  int                 in;
  int                 out;
};

struct reactor_pool
{
  int                 ref;
  reactor_user        user;
  int                 out[2];
  int                 in[2];
  int                 worker_count;
  list                workers;
  int                 job_count;
  list                jobs;
};

static __thread reactor_pool pool = {0};

static void *reactor_pool_worker_thread(void *arg)
{
  reactor_pool_worker *worker = arg;
  reactor_pool_job *job;
  ssize_t n;

  while (1)
    {
      n = read(worker->in, &job, sizeof job);
      reactor_assert_int_equal(n, sizeof job);

      (void) reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_CALL, 0);

      n = write(worker->out, &job, sizeof job);
      reactor_assert_int_equal(n, sizeof job);
    }

  return NULL;
}

static void reactor_pool_release_worker(void *item)
{
  reactor_pool_worker *worker = item;

  pool.worker_count --;
  pthread_cancel(worker->thread);
  pthread_join(worker->thread, NULL);
}

static void reactor_pool_grow(void)
{
  reactor_pool_worker *worker;
  int e;

  if (pool.worker_count >= REACTOR_POOL_WORKER_COUNT_MAX)
    return;

  worker = list_push_back(&pool.workers, NULL, sizeof (reactor_pool_worker));
  worker->in = pool.out[0];
  worker->out = pool.in[1];
  e = pthread_create(&worker->thread, NULL, reactor_pool_worker_thread, worker);
  reactor_assert_int_equal(e, 0);
  pool.worker_count ++;
}

static reactor_status reactor_pool_receive(reactor_event *event)
{
  reactor_pool_job *job;
  ssize_t n;

  reactor_assert_int_equal(event->data, EPOLLIN);
  while (1)
    {
      n = read(pool.in[0], &job, sizeof job);
      if (n == -1)
        {
          reactor_assert_int_equal(errno, EAGAIN);
          return REACTOR_OK;
        }
      reactor_assert_int_equal(n, sizeof job);

      (void) reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_RETURN, 0);

      list_erase(job, NULL);
      pool.job_count --;
      if (!pool.job_count)
        reactor_core_delete(&pool.user, pool.in[0]);
    }

  return REACTOR_OK;
}

void reactor_pool_construct(void)
{
  int e;

  if (!pool.ref)
    {
      reactor_user_construct(&pool.user, reactor_pool_receive, NULL);

      e = pipe(pool.out);
      reactor_assert_int_not_equal(e, -1);

      e = pipe(pool.in);
      reactor_assert_int_not_equal(e, -1);

      e = fcntl(pool.in[0], F_SETFL, O_NONBLOCK);
      reactor_assert_int_not_equal(e, -1);

      pool.worker_count = 0;
      list_construct(&pool.workers);

      pool.job_count = 0;
      list_construct(&pool.jobs);
    }

  pool.ref ++;
}

void reactor_pool_destruct(void)
{
  if (!pool.ref)
    return;

  pool.ref --;
  if (!pool.ref)
    {
      list_destruct(&pool.workers, reactor_pool_release_worker);
      list_destruct(&pool.jobs, NULL);

      close(pool.out[0]);
      close(pool.out[1]);
      close(pool.in[0]);
      close(pool.in[1]);
    }
}

void reactor_pool_dispatch(reactor_user_callback *callback, void *state)
{
  reactor_pool_job *job;
  ssize_t n;

  if (pool.job_count >= pool.worker_count)
    reactor_pool_grow();

  job = list_push_back(&pool.jobs, NULL, sizeof *job);
  reactor_user_construct(&job->user, callback, state);

  n = write(pool.out[1], &job, sizeof job);
  reactor_assert_int_equal(n, sizeof job);

  if (!pool.job_count)
    reactor_core_add(&pool.user, pool.in[0], EPOLLIN);

  pool.job_count ++;
}
