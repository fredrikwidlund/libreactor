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

static int server_ssl_activated = 0;

/* server date */

static __thread char server_date_string[30];

static data server_date(void)
{
  return data_construct(server_date_string, 29);
}

static void server_date_update(void)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  t = time(NULL);
  (void) gmtime_r(&t, &tm);
  (void) strftime(server_date_string, sizeof server_date_string, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(server_date_string, days[tm.tm_wday], 3);
  memcpy(server_date_string + 8, months[tm.tm_mon], 3);
}

/* server_transaction */

/*
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

*/

/* server_connection */

/*
static void server_connection_release(server_connection *connection)
{
  connection->transaction->state = SERVER_TRANSACTION_ABORT;
  connection->transaction->connection = NULL;
  server_transaction_release(connection->transaction);
  stream_destruct(&connection->stream);
  list_erase(connection, NULL);
}

*/

/* server */


/* common routines */

static void server_update(server_request *request)
{
  ssize_t n;

  request->fields_count = sizeof request->fields / sizeof request->fields[0];
  n = http_read_request(&request->stream, &request->method, &request->target, request->fields, &request->fields_count);
  switch (n)
  {
  case -2:
    server_close(request);
    return;
  case -1:
    return;
  default:
    server_hold(request); // hold to allow for consume
    server_hold(request); // hold for user
    reactor_dispatch(&request->handler, SERVER_REQUEST, (uintptr_t) request);
    if (request->active)
      stream_consume(&request->stream, n);
    server_release(request); // release consume hold
    break;
  }
}

static void server_request_callback(reactor_event *event)
{
  server_request *request = event->state;

  switch (event->type)
  {
  case STREAM_READ:
    server_update(request);
    break;
  default:
  case STREAM_CLOSE:
    server_close(request);
    break;
  }
}

static void server_timer_callback(reactor_event *event)
{
  assert(event->type == TIMER_EXPIRATION);
  server_date_update();
}

/* plain socket routines */

static void server_socket_accept(server *server, int fd)
{
  server_request *request;

  request = list_push_back(&server->requests, NULL, sizeof *request);
  *request = (server_request) {.ref = 1, .handler = server->handler, .active = 1};
  stream_construct(&request->stream, server_request_callback, request);
  stream_open(&request->stream, fd, STREAM_READ, NULL);
}

static void server_socket_descriptor_callback(reactor_event *event)
{
  server *server = event->state;
  int fd;

  assert(event->type == DESCRIPTOR_READ);
  while (1)
  {
    fd = accept4(descriptor_fd(&server->descriptor), NULL, NULL, SOCK_NONBLOCK);
    if (fd == -1)
      break;
    server_socket_accept(server, fd);
  }
}

static void server_socket_open(server *server, int fd)
{
  timer_set(&server->timer, 0, 1000000000);
  descriptor_construct(&server->descriptor, server_socket_descriptor_callback, server);
  descriptor_open(&server->descriptor, fd, DESCRIPTOR_READ);
}

/* ssl socket routines */

void server_ssl_accept(server *server, int fd)
{
  server_request *request;

  assert(server_ssl_activated == 1);
  
  request = list_push_back(&server->requests, NULL, sizeof *request);
  *request = (server_request) {.ref = 1, .handler = server->handler};
  stream_construct(&request->stream, server_request_callback, request);
  stream_open(&request->stream, fd, STREAM_READ, server->ssl_ctx);
}

static void server_ssl_descriptor_callback(reactor_event *event)
{
  server *server = event->state;
  int fd;

  assert(server_ssl_activated == 1);
  assert(event->type == DESCRIPTOR_READ);
  while (1)
  {
    fd = accept4(descriptor_fd(&server->descriptor), NULL, NULL, SOCK_NONBLOCK);
    if (fd == -1)
      break;
    server_ssl_accept(server, fd);
  }
}

static void server_ssl_open(server *server, int fd, SSL_CTX *ssl_ctx)
{
  server_ssl_activated = 1; 
  server->ssl_ctx = ssl_ctx;
  timer_set(&server->timer, 0, 1000000000);
  descriptor_construct(&server->descriptor, server_ssl_descriptor_callback, server);  
  descriptor_open(&server->descriptor, fd, DESCRIPTOR_READ);
}

/* public */

void server_construct(server *server, reactor_callback *callback, void *state)
{
  *server = (struct server) {0};
  reactor_handler_construct(&server->handler, callback, state);
  timer_construct(&server->timer, server_timer_callback, server);
  list_construct(&server->requests);
}

void server_destruct(server *server)
{
  server_shutdown(server);
  if (server_ssl_activated)
    SSL_CTX_free(server->ssl_ctx);
  timer_destruct(&server->timer);
  descriptor_destruct(&server->descriptor);
}

void server_open(server *server, int fd, SSL_CTX *ssl_ctx)
{
  if (ssl_ctx)
    server_ssl_open(server, fd, ssl_ctx);
  else
    server_socket_open(server, fd);
}

void server_shutdown(server *server)
{
  server_request *request, *next;

  request = list_front(&server->requests);
  while (request != list_end(&server->requests))
  {
    next = list_next(request);
    server_release(request);
    request = next;
  }
  
  timer_clear(&server->timer);
  descriptor_close(&server->descriptor);
}

void server_accept(server *server, int fd)
{
  if (server_ssl_activated)
  {
    server_ssl_accept(server, fd);
  }
  else
    server_socket_accept(server, fd);
}

void server_close(server_request *request)
{
  if (request->active)
  {
    request->active = 0;
    stream_close(&request->stream);
  }
  server_release(request);
}

void server_hold(server_request *request)
{
  request->ref++;
}

void server_release(server_request *request)
{
  request->ref--;
  if (request->ref)
    return;

  stream_destruct(&request->stream);
  list_erase(request, NULL);
}

void server_ok(server_request *request, data type, data body)
{
  if (request->active)
  {
    http_write_response(&request->stream, data_string("200 OK"), server_date(), type, body);
    stream_flush(&request->stream);
  }
  server_release(request);
}

void server_not_found(server_request *request)
{
  if (request->active)
  {
    http_write_response(&request->stream, data_string("404 Not Found"), server_date(), data_null(), data_null());
    stream_flush(&request->stream);
  }
  server_release(request);
}
