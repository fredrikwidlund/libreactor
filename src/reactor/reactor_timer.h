#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

enum reactor_timer_events
{
  REACTOR_TIMER_EVENT_ERROR,
  REACTOR_TIMER_EVENT_CALL,
  REACTOR_TIMER_EVENT_CLOSE
};

enum reactor_timer_state
{
  REACTOR_TIMER_STATE_CLOSED = 0x01,
  REACTOR_TIMER_STATE_ERROR  = 0x02,
  REACTOR_TIMER_STATE_OPEN   = 0x04
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  int           ref;
  int           state;
  reactor_user  user;
  int           fd;
};

void reactor_timer_open(reactor_timer *, reactor_user_callback *, void *, uint64_t, uint64_t);
void reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
void reactor_timer_close(reactor_timer *);

#endif /* REACTOR_TIMER_H_INCLUDED */
