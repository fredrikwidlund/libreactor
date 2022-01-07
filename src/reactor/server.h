#ifndef REACTOR_SERVER_H_INCLUDED
#define REACTOR_SERVER_H_INCLUDED

#include <openssl/ssl.h>

#include "reactor.h"

enum
{
  SERVER_REQUEST
};

enum
{
  SERVER_REQUEST_READY
};

typedef struct server             server;
typedef struct server_request     server_request;

struct server
{
  reactor_handler     handler;
  descriptor          descriptor;
  timer               timer;
  SSL_CTX            *ssl_ctx;
  list                requests;
};

struct server_request
{
  size_t              ref;
  int                 state;
  int                 active;
  reactor_handler     handler;
  stream              stream;
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
void server_ok(server_request *, data, data);
void server_not_found(server_request *);

/*
void server_transaction_ready(server_transaction *);
void server_transaction_write(server_transaction *, data);
void server_transaction_ok(server_transaction *, data, data);
void server_transaction_text(server_transaction *, data);
void server_transaction_printf(server_transaction *, data, const char *, ...);
void server_transaction_not_found(server_transaction *);
void server_transaction_disconnect(server_transaction *);
*/

#endif /* REACTOR_SERVER_H_INCLUDED */
