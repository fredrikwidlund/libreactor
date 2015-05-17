#ifndef REACTOR_CLIENT_H_INCLUDED
#define REACTOR_CLIENT_H_INCLUDE

#define REACTOR_CLIENT_CONNECTED 1

enum reactor_client_state
{
  REACTOR_CLIENT_STATE_INIT = 0,
  REACTOR_CLIENT_STATE_RESOLVING,
  REACTOR_CLIENT_STATE_RESOLVED,
  REACTOR_CLIENT_STATE_CONNECTING,
  REACTOR_CLIENT_STATE_CONNECTED,
  REACTOR_CLIENT_STATE_ERROR
};

typedef struct reactor_client reactor_client;
typedef enum reactor_client_state reactor_client_state;

struct reactor_client
{
  reactor_call          user;
  reactor              *reactor;
  reactor_client_state  state;
  int                   type;
  char                 *name;
  char                 *service;
  reactor_resolver     *resolver;
  int                   fd;
  reactor_socket       *socket;
};

reactor_client  *reactor_client_new(reactor *, reactor_handler *, int, char *, char *, void *);
int              reactor_client_construct(reactor *, reactor_client *, reactor_handler *, int, char *, char *, void *);
int              reactor_client_destruct(reactor_client *);
int              reactor_client_delete(reactor_client *);
void             reactor_client_handler(reactor_event *);
int              reactor_client_update(reactor_client *);
void             reactor_client_error(reactor_client *);
int              reactor_client_resolve(reactor_client *);
int              reactor_client_connect(reactor_client *);

#endif /* REACTOR_CLIENT_H_INCLUDED */

