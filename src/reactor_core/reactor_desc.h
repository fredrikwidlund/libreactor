#ifndef REACTOR_DESC_H_INCLUDED
#define REACTOR_DESC_H_INCLUDED

enum REACTOR_DESC_EVENTS
{
  REACTOR_DESC_ERROR = 0x01,
  REACTOR_DESC_READ  = 0x02,
  REACTOR_DESC_WRITE = 0x04,
  REACTOR_DESC_CLOSE = 0x08
};

enum REACTOR_DESC_STATE
{
  REACTOR_DESC_CLOSED = 0,
  REACTOR_DESC_OPEN
};

typedef struct reactor_desc reactor_desc;
struct reactor_desc
{
  int          state;
  short        events;
  short        events_saved;
  int          fd;
  reactor_user user;
};

void reactor_desc_init(reactor_desc *, reactor_user_call *, void *);
int  reactor_desc_open(reactor_desc *, int);
int  reactor_desc_fd(reactor_desc *);
void reactor_desc_events(reactor_desc *, int);
void reactor_desc_event(reactor_desc *, int);
int  reactor_desc_update(reactor_desc *);
void reactor_desc_close(reactor_desc *);
void reactor_desc_scheduled_close(reactor_desc *);
void reactor_desc_call(void *, int, void *);

#endif /* REACTOR_DESC_H_INCLUDED */
