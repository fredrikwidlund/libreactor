#ifndef REACTOR_TCP_H_INCLUDED
#define REACTOR_TCP_H_INCLUDED

enum reactor_tcp_event
{
  REACTOR_TCP_EVENT_ERROR,
  REACTOR_TCP_EVENT_ACCEPT,
  REACTOR_TCP_EVENT_CONNECT
};

enum reactor_tcp_flag
{
  REACTOR_TCP_FLAG_CLIENT = 0x00,
  REACTOR_TCP_FLAG_SERVER = 0x01
};

typedef struct reactor_tcp reactor_tcp;
struct reactor_tcp
{
  reactor_user       user;
  reactor_resolver   resolver;
  reactor_descriptor descriptor;
  int                flags;
};

int  reactor_tcp_open(reactor_tcp *, reactor_user_callback *, void *, char *, char *, int);
int  reactor_tcp_open_addrinfo(reactor_tcp *, reactor_user_callback *, void *, struct addrinfo *, int);
void reactor_tcp_close(reactor_tcp *);

#endif /* REACTOR_TCP_H_INCLUDED */
