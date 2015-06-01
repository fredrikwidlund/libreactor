#ifndef REACTOR_FD_H_INCLUDED
#define REACTOR_FD_H_INCLUDED

#define REACTOR_FD_READ   1
#define REACTOR_FD_WRITE  2
#define REACTOR_FD_ERROR  3
#define REACTOR_FD_CLOSE  4
#define REACTOR_FD_DELETE 5

#define REACTOR_FD_FLAG_ALLOCATED 0x01
#define REACTOR_FD_FLAG_ACTIVE    0x02
#define REACTOR_FD_FLAG_DELETE    0x04

typedef struct reactor_fd reactor_fd;

struct reactor_fd
{
  int                 flags;
  int                 descriptor;
  reactor            *reactor;
  struct epoll_event  ev;
  reactor_user        user;
  reactor_user        main;
};

reactor_fd *reactor_fd_new(reactor *, int, reactor_handler *, void *);
int         reactor_fd_init(reactor *, reactor_fd *, int, reactor_handler *, void *);
int         reactor_fd_destruct(reactor_fd *);
int         reactor_fd_delete(reactor_fd *);
void        reactor_fd_handler(void *, reactor_event *);
int         reactor_fd_events(reactor_fd *, int);

#endif /* REACTOR_FD_H_INCLUDED */
