#ifndef REACTOR_REACTOR_SERVER_H_INCLUDED
#define REACTOR_REACTOR_SERVER_H_INCLUDED

enum reactor_server_event
{
 REACTOR_SERVER_EVENT_ERROR,
 REACTOR_SERVER_EVENT_WARNING,
 REACTOR_SERVER_EVENT_REQUEST,
 REACTOR_SERVER_EVENT_CLOSE
};

enum reactor_server_session_flags
{
 REACTOR_SERVER_SESSION_READY      = 0x01,
 REACTOR_SERVER_SESSION_HANDLED    = 0x02,
 REACTOR_SERVER_SESSION_REGISTERED = 0x04,
 REACTOR_SERVER_SESSION_CORS       = 0x08,
 REACTOR_SERVER_SESSION_CLOSE      = 0x10
};

typedef struct reactor_server_session reactor_server_session;
typedef struct reactor_server         reactor_server;

struct reactor_server_session
{
  reactor_http          http;
  reactor_user          user;
  int                   flags;
  reactor_server       *server;
  reactor_http_request *request;
};

struct reactor_server
{
  reactor_user          user;
  reactor_net           net;
  list                  routes;
  list                  sessions;
};

void           reactor_server_construct(reactor_server *, reactor_user_callback *, void *);
void           reactor_server_destruct(reactor_server *);
reactor_status reactor_server_open(reactor_server *, char *, char *);
void           reactor_server_route(reactor_server *, reactor_user_callback *, void *);
void           reactor_server_close(reactor_server_session *);
void           reactor_server_register(reactor_server_session *, reactor_user_callback *, void *);
void           reactor_server_respond(reactor_server_session *, reactor_http_response *);
void           reactor_server_ok(reactor_server_session *, reactor_vector, reactor_vector);
void           reactor_server_found(reactor_server_session *, reactor_vector);
void           reactor_server_not_found(reactor_server_session *);

#endif /* REACTOR_REACTOR_SERVER_H_INCLUDED */
