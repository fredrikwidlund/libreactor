#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

enum reactor_timer_event
{
  REACTOR_TIMER_EVENT_ERROR,
  REACTOR_TIMER_EVENT_CALL
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  reactor_user       user;
  reactor_descriptor descriptor;
};

int  reactor_timer_open(reactor_timer *, reactor_user_callback *, void *, uint64_t, uint64_t);
int  reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
void reactor_timer_close(reactor_timer *);

#endif /* REACTOR_TIMER_H_INCLUDED */
