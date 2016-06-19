#ifndef REACTOR_TIMER_H_INCLUDED
#define REACTOR_TIMER_H_INCLUDED

#ifndef REACTOR_TIMER_BLOCK_SIZE
#define REACTOR_TIMER_BLOCK_SIZE 65536
#endif

enum reactor_timer_state
{
  REACTOR_TIMER_CLOSED,
  REACTOR_TIMER_OPEN,
  REACTOR_TIMER_INVALID,
  REACTOR_TIMER_CLOSE_WAIT,
};

enum reactor_timer_events
{
  REACTOR_TIMER_ERROR,
  REACTOR_TIMER_SIGNAL,
  REACTOR_TIMER_CLOSE
};

typedef struct reactor_timer reactor_timer;
struct reactor_timer
{
  int           state;
  reactor_user  user;
  reactor_desc  desc;
  int           ref;
};

void   reactor_timer_init(reactor_timer *, reactor_user_callback *, void *);
void   reactor_timer_open(reactor_timer *, uint64_t, uint64_t);
void   reactor_timer_set(reactor_timer *, uint64_t, uint64_t);
void   reactor_timer_close(reactor_timer *);
void   reactor_timer_event(void *, int, void *);
void   reactor_timer_error(reactor_timer *);

#endif /* REACTOR_TIMER_H_INCLUDED */
