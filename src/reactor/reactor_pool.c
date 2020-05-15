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

static __thread reactor_pool r_pool = {0};

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

  r_pool.worker_count --;
  pthread_cancel(worker->thread);
  pthread_join(worker->thread, NULL);
}

static void reactor_pool_grow(void)
{
  reactor_pool_worker *worker;
  int e;

  if (r_pool.worker_count >= REACTOR_POOL_WORKER_COUNT_MAX)
    return;

  worker = list_push_back(&r_pool.workers, NULL, sizeof (reactor_pool_worker));
  worker->in = r_pool.out[0];
  worker->out = r_pool.in[1];
  e = pthread_create(&worker->thread, NULL, reactor_pool_worker_thread, worker);
  reactor_assert_int_equal(e, 0);
  r_pool.worker_count ++;
}

static reactor_status reactor_pool_receive(reactor_event *event)
{
  reactor_pool_job *job;
  ssize_t n;

  reactor_assert_int_equal(event->data, EPOLLIN);
  while (1)
    {
      n = read(r_pool.in[0], &job, sizeof job);
      if (n == -1)
        {
          reactor_assert_int_equal(errno, EAGAIN);
          return REACTOR_OK;
        }
      reactor_assert_int_equal(n, sizeof job);

      (void) reactor_user_dispatch(&job->user, REACTOR_POOL_EVENT_RETURN, 0);

      list_erase(job, NULL);
      r_pool.job_count --;
      if (!r_pool.job_count)
        reactor_core_delete(&r_pool.user, r_pool.in[0]);
    }

  return REACTOR_OK;
}

void reactor_pool_construct(void)
{
  int e;

  if (!r_pool.ref)
    {
      reactor_user_construct(&r_pool.user, reactor_pool_receive, NULL);

      e = pipe(r_pool.out);
      reactor_assert_int_not_equal(e, -1);

      e = pipe(r_pool.in);
      reactor_assert_int_not_equal(e, -1);

      e = fcntl(r_pool.in[0], F_SETFL, O_NONBLOCK);
      reactor_assert_int_not_equal(e, -1);

      r_pool.worker_count = 0;
      list_construct(&r_pool.workers);

      r_pool.job_count = 0;
      list_construct(&r_pool.jobs);
    }

  r_pool.ref ++;
}

void reactor_pool_destruct(void)
{
  if (!r_pool.ref)
    return;

  r_pool.ref --;
  if (!r_pool.ref)
    {
      list_destruct(&r_pool.workers, reactor_pool_release_worker);
      list_destruct(&r_pool.jobs, NULL);

      close(r_pool.out[0]);
      close(r_pool.out[1]);
      close(r_pool.in[0]);
      close(r_pool.in[1]);
    }
}

void reactor_pool_dispatch(reactor_user_callback *callback, void *state)
{
  reactor_pool_job *job;
  ssize_t n;

  if (r_pool.job_count >= r_pool.worker_count)
    reactor_pool_grow();

  job = list_push_back(&r_pool.jobs, NULL, sizeof *job);
  reactor_user_construct(&job->user, callback, state);

  n = write(r_pool.out[1], &job, sizeof job);
  reactor_assert_int_equal(n, sizeof job);

  if (!r_pool.job_count)
    reactor_core_add(&r_pool.user, r_pool.in[0], EPOLLIN);

  r_pool.job_count ++;
}
