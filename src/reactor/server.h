#ifndef REACTOR_SERVER_H_INCLUDED
#define REACTOR_SERVER_H_INCLUDED

#include <openssl/ssl.h>

#include "list.h"
#include "reactor.h"
#include "stream.h"
#include "descriptor.h"
#include "timer.h"

enum
{
  SERVER_TRANSACTION_READ_REQUEST,
  SERVER_TRANSACTION_HANDLE_REQUEST,
  SERVER_TRANSACTION_ABORT
};

enum
{
  SERVER_TRANSACTION
};

typedef struct server             server;
typedef struct server_connection  server_connection;
typedef struct server_transaction server_transaction;

struct server
{
  reactor_handler    handler;
  descriptor         descriptor;
  timer              timer;
  char               date[30];
  list               connections;
  SSL_CTX           *ssl_ctx;
};

struct server_transaction
{
  size_t             ref;
  int                state;
  server_connection *connection;
};

struct server_connection
{
  server             *server;
  stream              stream;
  server_transaction *transaction;
};

string server_date(server *);
void   server_construct(server *, reactor_callback *, void *);
void   server_destruct(server *);
void   server_open(server *, int, SSL_CTX *);
void   server_accept(server *, int, SSL_CTX *);
void   server_shutdown(server *);
void   server_close(server *);

void   server_transaction_ok(server_transaction *, string, const void *, size_t);
void   server_transaction_text(server_transaction *, string);
void   server_transaction_printf(server_transaction *, string, const char *, ...);
void   server_transaction_disconnect(server_transaction *);

#endif /* REACTOR_SERVER_H_INCLUDED */
