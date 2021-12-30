#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "data.h"
#include "utility.h"
#include "reactor.h"
#include "descriptor.h"
#include "stream.h"
#include "timer.h"
#include "http.h"
#include "server.h"

data server_date(server *server)
{
  return data_construct(server->date, 29);
}

static void server_date_update(server *server)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  t = time(NULL);
  (void) gmtime_r(&t, &tm);
  (void) strftime(server->date, sizeof server->date, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(server->date, days[tm.tm_wday], 3);
  memcpy(server->date + 8, months[tm.tm_mon], 3);
}

/* prototypes */

static void server_connection_release(server_connection *);

/* server_transaction */

static server_transaction *server_transaction_new(server_connection *connection)
{
  server_transaction *transaction = malloc(sizeof *transaction);

  *transaction = (server_transaction) {.ref = 1, .state = SERVER_TRANSACTION_READ_REQUEST, .connection = connection};
  return transaction;
}

static void server_transaction_hold(server_transaction *transaction)
{
  transaction->ref++;
}

static void server_transaction_release(server_transaction *transaction)
{
  transaction->ref--;
  if (!transaction->ref)
    free(transaction);
}

static void server_transaction_update(server_transaction *transaction)
{
  data data;
  ssize_t n;
  http_request request;

  if (transaction->state != SERVER_TRANSACTION_READ_REQUEST)
    return;

  data = stream_read(&transaction->connection->stream);
  n = http_request_parse(&request, data);
  if (reactor_likely(n > 0))
  {
    transaction->state = SERVER_TRANSACTION_HANDLE_REQUEST;
    transaction->request = &request;
    server_transaction_hold(transaction);
    reactor_dispatch(&transaction->connection->server->handler, SERVER_TRANSACTION, (uintptr_t) transaction);
    if (transaction->connection)
    {
      stream_consume(&transaction->connection->stream, n);
      stream_flush(&transaction->connection->stream);
    }
    server_transaction_release(transaction);
  }
  else if (n == -1)
    server_connection_release(transaction->connection);
}

void server_transaction_ready(server_transaction *transaction)
{
  transaction->state = SERVER_TRANSACTION_READ_REQUEST;
}

void server_transaction_write(server_transaction *transaction, data data)
{
  stream_write(&transaction->connection->stream, data);
}

void server_transaction_ok(server_transaction *transaction, data type, data body)
{
  http_write_ok_response(&transaction->connection->stream, server_date(transaction->connection->server), type, body);
  server_transaction_ready(transaction);
}

void server_transaction_text(server_transaction *transaction, data text)
{
  http_write_ok_response(&transaction->connection->stream, server_date(transaction->connection->server),
                         data_string("text/plain"), text);
  server_transaction_ready(transaction);
}

void server_transaction_printf(server_transaction *transaction, data type, const char *format, ...)
{
  va_list ap;
  char *body;
  int n;

  va_start(ap, format);
  n = vasprintf(&body, format, ap);
  server_transaction_ok(transaction, type, data_construct(body, n));
  va_end(ap);
  free(body);
}

void server_transaction_not_found(server_transaction *transaction)
{
  http_write_response(&transaction->connection->stream, data_string("404 Not Found"),
                      server_date(transaction->connection->server), data_null(), data_null());
  server_transaction_ready(transaction);
}

void server_transaction_disconnect(server_transaction *transaction)
{
  server_connection_release(transaction->connection);
}

/* server_connection */

static void server_connection_release(server_connection *connection)
{
  connection->transaction->state = SERVER_TRANSACTION_ABORT;
  connection->transaction->connection = NULL;
  server_transaction_release(connection->transaction);
  stream_destruct(&connection->stream);
  list_erase(connection, NULL);
}

static void server_connection_callback(reactor_event *event)
{
  server_connection *connection = event->state;

  switch (event->type)
  {
  case STREAM_READ:
    server_transaction_update(connection->transaction);
    break;
  default:
  case STREAM_CLOSE:
    server_connection_release(connection);
    break;
  }
}

/* server */

static void server_new_connection(server *server, int fd, SSL_CTX *ssl_ctx)
{
  server_connection *connection;

  connection = list_push_back(&server->connections, NULL, sizeof *connection);
  connection->server = server;
  stream_construct(&connection->stream, server_connection_callback, connection);
  connection->transaction = server_transaction_new(connection);
  stream_open(&connection->stream, fd, ssl_ctx, 0);
}

static void server_descriptor(reactor_event *event)
{
  server *server = event->state;
  int fd;

  switch (event->type)
  {
  case DESCRIPTOR_READ:
    while (1)
    {
      fd = accept4(descriptor_fd(&server->descriptor), NULL, NULL, SOCK_NONBLOCK);
      if (fd == -1)
        break;
      server_accept(server, fd, server->ssl_ctx);
    }
    break;
  default:
  case DESCRIPTOR_CLOSE:
    server_close(server);
    break;
  }
}

static void server_timer(reactor_event *event)
{
  server *server = event->state;

  server_date_update(server);
}

void server_construct(server *server, reactor_callback *callback, void *state)
{
  *server = (struct server) {0};
  reactor_handler_construct(&server->handler, callback, state);
  descriptor_construct(&server->descriptor, server_descriptor, server);
  timer_construct(&server->timer, server_timer, server);
  list_construct(&server->connections);
}

void server_destruct(server *server)
{
  server_close(server);
  SSL_CTX_free(server->ssl_ctx);
  descriptor_destruct(&server->descriptor);
}

void server_open(server *server, int fd, SSL_CTX *ssl_ctx)
{
  server->ssl_ctx = ssl_ctx;
  descriptor_open(&server->descriptor, fd, 0);
  timer_set(&server->timer, 0, 1000000000);
}

void server_accept(server *server, int fd, SSL_CTX *ssl_ctx)
{
  server_new_connection(server, fd, ssl_ctx);
}

void server_shutdown(server *server)
{
  int e;

  e = shutdown(descriptor_fd(&server->descriptor), SHUT_RDWR);
  assert(e == 0);
}

void server_close(server *server)
{
  timer_clear(&server->timer);
  descriptor_close(&server->descriptor);
  while (!list_empty(&server->connections))
    server_connection_release(list_front(&server->connections));
  list_destruct(&server->connections, NULL);
}
