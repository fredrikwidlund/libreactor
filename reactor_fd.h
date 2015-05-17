#ifndef REACTOR_FD_H_INCLUDED
#define REACTOR_FD_H_INCLUDED

#define REACTOR_FD_READ  0x01
#define REACTOR_FD_WRITE 0x02

typedef struct reactor_fd reactor_fd;

struct reactor_fd
{
  reactor_call        user;
  reactor            *reactor;
  int                 descriptor;
  struct epoll_event  ev;
  reactor_call        main;
};

reactor_fd *reactor_fd_new(reactor *, int, void *, reactor_handler *, void *);
int         reactor_fd_construct(reactor *, reactor_fd *, int, void *, reactor_handler *, void *);
int         reactor_fd_destruct(reactor_fd *);
int         reactor_fd_delete(reactor_fd *);
void        reactor_fd_handler(reactor_event *);
int         reactor_fd_descriptor(reactor_fd *);
int         reactor_fd_events(reactor_fd *, int);

#endif /* REACTOR_FD_H_INCLUDED */
