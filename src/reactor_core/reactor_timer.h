#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

enum REACTOR_TIMER_EVENT
{
  REACTOR_TIMER_ERROR   = 0x00,
  REACTOR_TIMER_TIMEOUT = 0x01,
  REACTOR_TIMER_CLOSE   = 0x02
};

enum REACTOR_TIMER_STATE
{
  REACTOR_TIMER_CLOSED = 0,
  REACTOR_TIMER_OPEN,
  REACTOR_TIMER_CLOSING
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  reactor_desc desc;
  reactor_user user;
  int          state;
};

void reactor_timer_init(reactor_timer *, reactor_user_call *, void *);
int  reactor_timer_open(reactor_timer *, uint64_t, uint64_t);
int  reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
void reactor_timer_close(reactor_timer *);
void reactor_timer_event(void *, int, void *);

#endif /* REACTOR_TIMER_H_INCLUDED */
