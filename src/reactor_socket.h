#ifndef REACTOR_SOCKET_H_INCLUDED
#define REACTOR_SOCKET_H_INCLUDED

enum reactor_socket_event
{
  REACTOR_SOCKET_ERROR = 0,
  REACTOR_SOCKET_READ,
  REACTOR_SOCKET_WRITE,
  REACTOR_SOCKET_CLOSE
};

typedef struct reactor_socket reactor_socket;

struct reactor_socket
{
  reactor_call        user;
  reactor            *reactor;
  reactor_fd          fd;
  buffer              output_buffer;
};

reactor_socket *reactor_socket_new(reactor *, int, void *, reactor_handler *, void *);
int             reactor_socket_construct(reactor *, reactor_socket *, int, void *, reactor_handler *, void *);
int             reactor_socket_destruct(reactor_socket *);
int             reactor_socket_delete(reactor_socket *);
void            reactor_socket_handler(reactor_event *);
int             reactor_socket_write_notify(reactor_socket *);
int             reactor_socket_write(reactor_socket *, char *, size_t);
void            reactor_socket_flush(reactor_socket *);

#endif /* REACTOR_SOCKET_H_INCLUDED */

