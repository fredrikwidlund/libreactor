#ifndef REACTOR_POOL_H_INCLUDED
#define REACTOR_POOL_H_INCLUDED

#ifndef REACTOR_POOL_STACK_SIZE
#define REACTOR_POOL_STACK_SIZE 65536
#endif

enum reactor_pool_event
{
  REACTOR_POOL_EVENT_CALL,
  REACTOR_POOL_EVENT_RETURN
};

typedef struct reactor_pool_job reactor_pool_job;
struct reactor_pool_job
{
  reactor_user       user;
};

typedef struct reactor_pool_worker reactor_pool_worker;
struct reactor_pool_worker
{
  pid_t              pid;
  void              *stack;
};

typedef struct reactor_pool reactor_pool;
struct reactor_pool
{
  int                queue[2];
  size_t             jobs;
  vector             workers;
};

void reactor_pool_construct(reactor_pool *);
void reactor_pool_create_job(reactor_pool *, reactor_user_callback *, void *);

#endif /* REACTOR_POOL_H_INCLUDED */
