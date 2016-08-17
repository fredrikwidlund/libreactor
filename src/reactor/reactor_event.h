#ifndef REACTOR_EVENT_H_INCLUDED
#define REACTOR_EVENT_H_INCLUDED

enum reactor_event_state
{
  REACTOR_EVENT_CLOSED,
  REACTOR_EVENT_OPEN,
  REACTOR_EVENT_INVALID,
  REACTOR_EVENT_CLOSE_WAIT,
};

enum reactor_event_events
{
  REACTOR_EVENT_ERROR,
  REACTOR_EVENT_SIGNAL,
  REACTOR_EVENT_SHUTDOWN,
  REACTOR_EVENT_CLOSE
};

typedef struct reactor_event reactor_event;
struct reactor_event
{
  int           state;
  reactor_user  user;
  reactor_desc  desc;
  int           ref;
};

void   reactor_event_init(reactor_event *, reactor_user_callback *, void *);
void   reactor_event_open(reactor_event *);
void   reactor_event_close(reactor_event *);
void   reactor_event_event(void *, int, void *);
void   reactor_event_error(reactor_event *);

#endif /* REACTOR_EVENT_H_INCLUDED */
