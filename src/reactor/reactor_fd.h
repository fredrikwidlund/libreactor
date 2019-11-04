#ifndef REACTOR_REACTOR_FD_H_INCLUDED
#define REACTOR_REACTOR_FD_H_INCLUDED

enum reactor_fd_events
{
  REACTOR_FD_EVENT
};

typedef struct reactor_fd reactor_fd;
struct reactor_fd
{
  reactor_user user;
  int          fileno;
};

void reactor_fd_construct(reactor_fd *, reactor_user_callback *, void *);
void reactor_fd_destruct(reactor_fd *);
int  reactor_fd_deconstruct(reactor_fd *);
void reactor_fd_open(reactor_fd *, int, int);
void reactor_fd_close(reactor_fd *);
void reactor_fd_events(reactor_fd *, int);
int  reactor_fd_active(reactor_fd *);
int  reactor_fd_fileno(reactor_fd *);

#endif /* REACTOR_REACTOR_FD_H_INCLUDED */
