#ifndef REACTOR_FD_H_INCLUDED
#define REACTOR_FD_H_INCLUDED

#define REACTOR_FD_READ 1

typedef struct reactor_fd reactor_fd;

struct reactor_fd
{
  reactor_call        user;
  reactor            *reactor;
  int                 descriptor;
  struct epoll_event  ev;
  reactor_call        main;
};

reactor_fd *reactor_fd_new(reactor *, reactor_handler *, int, void *);
int         reactor_fd_construct(reactor *, reactor_fd *, reactor_handler *, int, void *);
int         reactor_fd_destruct(reactor_fd *);
int         reactor_fd_delete(reactor_fd *);
void        reactor_fd_handler(reactor_event *);
int         reactor_fd_descriptor(reactor_fd *);

#endif /* REACTOR_FD_H_INCLUDED */
