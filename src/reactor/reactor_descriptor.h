#ifndef REACTOR_DESCRIPTOR_H_INCLUDED
#define REACTOR_DESCRIPTOR_H_INCLUDED

enum reactor_descriptor_event
{
  REACTOR_DESCRIPTOR_EVENT_POLL
};

enum reactor_descriptor_state
{
  REACTOR_DESCRIPTOR_STATE_CLOSED = 0,
  REACTOR_DESCRIPTOR_STATE_OPEN
};

typedef struct reactor_descriptor reactor_descriptor;
struct reactor_descriptor
{
  reactor_user user;
  int          state;
  int          fd;
};

int  reactor_descriptor_open(reactor_descriptor *, reactor_user_callback *, void *, int, int);
void reactor_descriptor_close(reactor_descriptor *);
int  reactor_descriptor_clear(reactor_descriptor *);
int  reactor_descriptor_event(reactor_descriptor *, int);
void reactor_descriptor_set(reactor_descriptor *, int);
int  reactor_descriptor_fd(reactor_descriptor *);

#endif /* REACTOR_DESCRIPTOR_H_INCLUDED */
