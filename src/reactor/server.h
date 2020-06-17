#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

enum
{
  SERVER_ERROR,
  SERVER_REQUEST
};

typedef struct server         server;
typedef struct server_context server_context;
typedef struct server_session server_session;

struct server
{
  core_handler    user;
  timer           timer;
  int             fd;
  int             next;
  list            sessions;
};

struct server_context
{
  server_session *session;
  http_request    request;
};

struct server_session
{
  stream          stream;
  server         *server;
};

void server_construct(server *, core_callback *, void *);
void server_open(server *, uint32_t, uint16_t);
void server_close(server *);
void server_destruct(server *);
void server_ok(server_context *, segment, segment);

#endif /* SERVER_H_INCLUDED */
