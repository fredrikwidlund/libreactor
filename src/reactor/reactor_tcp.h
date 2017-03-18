#ifndef REACTOR_TCP_H_INCLUDED
#define REACTOR_TCP_H_INCLUDED

enum reactor_tcp_state
{
  REACTOR_TCP_STATE_CLOSED     = 0x01,
  REACTOR_TCP_STATE_CLOSING    = 0x02,
  REACTOR_TCP_STATE_RESOLVING  = 0x04,
  REACTOR_TCP_STATE_CONNECTING = 0x08,
  REACTOR_TCP_STATE_ACCEPTING  = 0x10,
  REACTOR_TCP_STATE_ERROR      = 0x20
};

enum reactor_tcp_event
{
  REACTOR_TCP_EVENT_ERROR,
  REACTOR_TCP_EVENT_ACCEPT,
  REACTOR_TCP_EVENT_CONNECT,
  REACTOR_TCP_EVENT_CLOSE,
  REACTOR_TCP_EVENT_DESTRUCT
};

enum reactor_tcp_flag
{
  REACTOR_TCP_FLAG_SERVER = 0x01
};

typedef struct reactor_tcp reactor_tcp;
struct reactor_tcp
{
  short             ref;
  short             state;
  reactor_user      user;
  int               socket;
  int               flags;
  reactor_resolver *resolver;
};

void reactor_tcp_hold(reactor_tcp *);
void reactor_tcp_release(reactor_tcp *);
void reactor_tcp_open(reactor_tcp *, reactor_user_callback *, void *, char *, char *, int);
void reactor_tcp_close(reactor_tcp *);

#endif /* REACTOR_TCP_H_INCLUDED */
