#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#define SERVER_OPTION_DEFAULTS 0x00

enum server_options
{
  SERVER_OPTION_BPF       = 0x01
};
typedef enum server_options server_options;

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
  server_options  options;
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
void server_option_set(server *, server_options);
void server_option_clear(server *, server_options);
void server_open(server *, uint32_t, uint16_t);
void server_close(server *);
void server_destruct(server *);
void server_ok(server_context *, segment, segment);

#endif /* SERVER_H_INCLUDED */
