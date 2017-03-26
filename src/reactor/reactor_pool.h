#ifndef REACTOR_POOL_H_INCLUDED
#define REACTOR_POOL_H_INCLUDED

#ifndef REACTOR_POOL_STACK_SIZE
#define REACTOR_POOL_STACK_SIZE 65536
#endif

#ifndef REACTOR_POOL_WORKERS_MAX
#define REACTOR_POOL_WORKERS_MAX 1024
#endif

enum reactor_pool_event
{
  REACTOR_POOL_EVENT_CALL,
  REACTOR_POOL_EVENT_RETURN
};

typedef struct reactor_pool_job reactor_pool_job;
struct reactor_pool_job
{
  size_t               job;
  reactor_user         user;
  TAILQ_ENTRY(reactor_pool_job)   entries;
};

typedef struct reactor_pool_worker reactor_pool_worker;
struct reactor_pool_worker
{
  pid_t                pid;
  void                *stack;
  TAILQ_ENTRY(reactor_pool_worker)   entries;
};

typedef struct reactor_pool reactor_pool;
struct reactor_pool
{
  int                  queue[2];
  TAILQ_HEAD(, reactor_pool_job)  jobs_head;
  size_t               jobs;
  TAILQ_HEAD(, reactor_pool_worker)  workers_head;
  size_t               workers;
  size_t               workers_min;
  size_t               workers_max;
};

void reactor_pool_construct(reactor_pool *);
void reactor_pool_destruct(reactor_pool *);
void reactor_pool_limits(reactor_pool *, size_t, size_t);
void reactor_pool_enqueue(reactor_pool *, reactor_user_callback *, void *);

#endif /* REACTOR_POOL_H_INCLUDED */
