#ifndef REACTOR_REACTOR_TIMER_H_INCLUDED
#define REACTOR_REACTOR_TIMER_H_INCLUDED

enum reactor_timer_event
{
  REACTOR_TIMER_EVENT_ERROR,
  REACTOR_TIMER_EVENT_ALARM
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  reactor_user user;
  reactor_fd   fd;
};

void reactor_timer_construct(reactor_timer *, reactor_user_callback *, void *);
void reactor_timer_destruct(reactor_timer *);
void reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
void reactor_timer_clear(reactor_timer *);

#endif /* REACTOR_REACTOR_TIMER_H_INCLUDED */
