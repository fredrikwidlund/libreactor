#ifndef REACTOR_HTTP_SERVER_H_INCLUDED
#define REACTOR_HTTP_SERVER_H_INCLUDED

enum reactor_http_server_events
{
  REACTOR_HTTP_SERVER_EVENT_ERROR,
  REACTOR_HTTP_SERVER_EVENT_CLOSE,
  REACTOR_HTTP_SERVER_EVENT_REQUEST
};

enum reactor_http_server_state
{
  REACTOR_HTTP_SERVER_STATE_CLOSED = 0x01,
  REACTOR_HTTP_SERVER_STATE_OPEN   = 0x02
};

enum reactor_http_server_map_types
{
  REACTOR_HTTP_SERVER_MAP_MATCH
};

typedef struct reactor_http_server_map reactor_http_server_map;
struct reactor_http_server_map
{
  int                          type;
  reactor_user                 user;
  reactor_memory               method;
  reactor_memory               path;
};

typedef struct reactor_http_server reactor_http_server;
struct reactor_http_server
{
  int                          ref;
  int                          state;
  reactor_user                 user;
  reactor_tcp                  tcp;
  reactor_timer                timer;
  vector                       map;
  char                        *name;
  char                         date[32];
};

typedef struct reactor_http_server_session reactor_http_server_session;
struct reactor_http_server_session
{
  reactor_http_server         *server;
  reactor_http                 http;
};

typedef struct reactor_http_server_context reactor_http_server_context;
struct reactor_http_server_context
{
  reactor_http_server_session *session;
  reactor_http_request        *request;
};

void reactor_http_server_open(reactor_http_server *, reactor_user_callback *, void *, char *, char *);
void reactor_http_server_close(reactor_http_server *);
void reactor_http_server_route(reactor_http_server *, reactor_user_callback *, void *, char *, char *);
void reactor_http_server_respond_mime(reactor_http_server_session *, char *, void *, size_t);
void reactor_http_server_respond_text(reactor_http_server_session *, char *);

#endif /* REACTOR_HTTP_SERVER_H_INCLUDED */
