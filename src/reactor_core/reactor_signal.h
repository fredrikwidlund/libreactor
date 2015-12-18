#ifndef REACTOR_SIGNAL_H_INCLUDED
#define REACTOR_SIGNAL_H_INCLUDED

enum REACTOR_SIGNAL_EVENT
{
  REACTOR_SIGNAL_ERROR = 0x00,
  REACTOR_SIGNAL_RAISE = 0x01,
  REACTOR_SIGNAL_CLOSE = 0x02
};

enum REACTOR_SIGNAL_STATE
{
  REACTOR_SIGNAL_CLOSED = 0,
  REACTOR_SIGNAL_OPEN,
  REACTOR_SIGNAL_CLOSING
};

typedef struct reactor_signal reactor_signal;
struct reactor_signal
{
  reactor_desc desc;
  reactor_user user;
  int          state;
};

void reactor_signal_init(reactor_signal *, reactor_user_call *, void *);
int  reactor_signal_open(reactor_signal *, sigset_t *);
void reactor_signal_close(reactor_signal *);
void reactor_signal_scheduled_close(reactor_signal *);
void reactor_signal_event(void *, int, void *);

#endif /* REACTOR_SIGNAL_H_INCLUDED */
