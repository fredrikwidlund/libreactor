#ifndef REACTOR_SERVER_H_INCLUDED
#define REACTOR_SERVER_H_INCLUDED

#include <openssl/ssl.h>

#include "reactor.h"

enum
{
  SERVER_REQUEST
};

enum server_state
{
  SERVER_REQUEST_NEED_MORE_DATA,
  SERVER_REQUEST_READ,
  SERVER_REQUEST_HANDLING,
  SERVER_REQUEST_CLOSED
};

typedef struct server             server;
typedef struct server_request     server_request;

struct server
{
  reactor_handler     handler;
  int                 is_open;
  descriptor          descriptor;
  timer               timer;
  SSL_CTX            *ssl_ctx;
  list                requests;
};

struct server_request
{
  size_t              ref;
  enum server_state   state;
  int                 event_triggered;
  reactor_handler     handler;
  stream              stream;
  data                data;
  data                method;
  data                target;
  http_field          fields[16];
  size_t              fields_count;
};

void server_construct(server *, reactor_callback *, void *);
void server_destruct(server *);
void server_open(server *, int, SSL_CTX *);
void server_shutdown(server *);
void server_accept(server *, int);

void server_close(server_request *);
void server_hold(server_request *);
void server_release(server_request *);
void server_respond(server_request *, data, data, data);
void server_ok(server_request *, data, data);
void server_not_found(server_request *);
void server_bad_request(server_request *);

#endif /* REACTOR_SERVER_H_INCLUDED */
