#ifndef REACTOR_TCP_H_INCLUDED
#define REACTOR_TCP_H_INCLUDED

enum reactor_tcp_state
{
  REACTOR_TCP_CLOSED,
  REACTOR_TCP_OPEN
};

enum reactor_tcp_events
{
  REACTOR_TCP_ERROR,
  REACTOR_TCP_ACCEPT,
  REACTOR_TCP_CONNECT,
  REACTOR_TCP_CLOSE
};

typedef struct reactor_tcp reactor_tcp;
struct reactor_tcp
{
  int          state;
  reactor_user user;
  reactor_desc desc;
};

void reactor_tcp_init(reactor_tcp *, reactor_user_callback *, void *);
void reactor_tcp_error(reactor_tcp *);
void reactor_tcp_close(reactor_tcp *);
void reactor_tcp_connect(reactor_tcp *, char *, char *);
void reactor_tcp_listen(reactor_tcp *, char *, char *);
void reactor_tcp_event(void *, int, void *);

/*
void reactor_tcp_server_event(void *, int, void *);
int  reactor_tcp_server_listen(reactor_tcp_server *, int, struct sockaddr *, socklen_t);
int  reactor_tcp_server_set_defer_accept(reactor_tcp_server *, int);
int  reactor_tcp_server_set_quickack(reactor_tcp_server *, int);
void reactor_tcp_server_error(reactor_tcp_server *);
void reactor_tcp_server_close(reactor_tcp_server *);
*/

#endif /* REACTOR_TCP_H_INCLUDED */
