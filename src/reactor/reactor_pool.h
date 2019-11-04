#ifndef REACTOR_REACTOR_POOL_H_INCLUDED
#define REACTOR_REACTOR_POOL_H_INCLUDED

#define REACTOR_POOL_WORKER_COUNT_MAX 32

enum reactor_pool_event_type
{
  REACTOR_POOL_EVENT_CALL,
  REACTOR_POOL_EVENT_RETURN
};

void reactor_pool_construct(void);
void reactor_pool_destruct(void);
void reactor_pool_dispatch(reactor_user_callback *, void *);

#endif /* REACTOR_REACTOR_POOL_H_INCLUDED */
