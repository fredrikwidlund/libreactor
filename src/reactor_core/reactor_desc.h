#ifndef REACTOR_DESC_H_INCLUDED
#define REACTOR_DESC_H_INCLUDED

enum reactor_desc_state
{
  REACTOR_DESC_CLOSED = 0,
  REACTOR_DESC_OPEN,
  REACTOR_DESC_INVALID,
  REACTOR_DESC_CLOSE_WAIT
};

enum reactor_desc_events
{
  REACTOR_DESC_ERROR,
  REACTOR_DESC_READ,
  REACTOR_DESC_WRITE,
  REACTOR_DESC_SHUTDOWN,
  REACTOR_DESC_CLOSE
};

typedef struct reactor_desc reactor_desc;
struct reactor_desc
{
  int          state;
  reactor_user user;
  int          index;
  int          ref;
};

void    reactor_desc_init(reactor_desc *, reactor_user_callback *, void *);
void    reactor_desc_open(reactor_desc *, int);
void    reactor_desc_close(reactor_desc *);
void    reactor_desc_close_final(reactor_desc *);
void    reactor_desc_error(reactor_desc *);
void    reactor_desc_events(reactor_desc *, int);
int     reactor_desc_fd(reactor_desc *);
void    reactor_desc_event(void *, int, void *);
ssize_t reactor_desc_read(reactor_desc *, void *, size_t);
void    reactor_desc_read_notify(reactor_desc *, int);
ssize_t reactor_desc_write(reactor_desc *, void *, size_t);
void    reactor_desc_write_notify(reactor_desc *, int);

#endif /* REACTOR_DESC_H_INCLUDED */
