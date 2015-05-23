#ifndef REACTOR_SOCKET_H_INCLUDED
#define REACTOR_SOCKET_H_INCLUDED

#define REACTOR_SOCKET_CLOSE       0
#define REACTOR_SOCKET_DATA        1
#define REACTOR_SOCKET_WRITE_READY 2

typedef struct reactor_socket reactor_socket;

struct reactor_socket
{
  reactor_call        user;
  reactor            *reactor;
  reactor_fd          fd;
};

reactor_socket *reactor_socket_new(reactor *, int, void *, reactor_handler *, void *);
int             reactor_socket_construct(reactor *, reactor_socket *, int, void *, reactor_handler *, void *);
int             reactor_socket_destruct(reactor_socket *);
int             reactor_socket_delete(reactor_socket *);
void            reactor_socket_handler(reactor_event *);
int             reactor_socket_write_notify(reactor_socket *);

#endif /* REACTOR_SOCKET_H_INCLUDED */

