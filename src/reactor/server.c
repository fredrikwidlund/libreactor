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

/* common routines */

static void server_request_read(server_request *request)
{
  ssize_t n;

  if (data_empty(request->data))
  {
    request->state = SERVER_REQUEST_NEED_MORE_DATA;
    return;
  }

  request->fields_count = sizeof request->fields / sizeof request->fields[0];
  n = http_read_request_data(request->data, &request->method, &request->target, request->fields, &request->fields_count);
  switch (n)
  {
  case -2:
    request->state = SERVER_REQUEST_HANDLING;
    server_hold(request);
    server_bad_request(request);
    break;
  case -1:
    request->state = SERVER_REQUEST_NEED_MORE_DATA;
    break;
  default:
    request->state = SERVER_REQUEST_HANDLING;
    server_hold(request);
    request->data = data_consume(request->data, n);
    reactor_dispatch(&request->handler, SERVER_REQUEST, (uintptr_t) request);
    break;
  }
}

static void server_request_update(server_request *request)
{
  if (request->state != SERVER_REQUEST_NEED_MORE_DATA || !request->stream.input.size)
    return;

  server_hold(request);
  request->state = SERVER_REQUEST_READ;
  request->event_triggered = 1;
  request->data = data_construct(request->stream.input.data, request->stream.input.size);

  while (request->state == SERVER_REQUEST_READ)
    server_request_read(request);

  if (request->state != SERVER_REQUEST_ABORT)
  {
    stream_consume(&request->stream, request->stream.input.size - request->data.size);
    stream_flush(&request->stream);
  }

  request->event_triggered = 0;
  server_release(request);
}

static void server_request_callback(reactor_event *event)
{
  server_request *request = event->state;

  switch (event->type)
  {
  case STREAM_READ:
    server_request_update(request);
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
  *request = (server_request) {.ref = 1, .handler = server->handler, .state = SERVER_REQUEST_NEED_MORE_DATA};
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

  request = list_push_back(&server->requests, NULL, sizeof *request);
  *request = (server_request) {.ref = 1, .handler = server->handler};
  stream_construct(&request->stream, server_request_callback, request);
  stream_open(&request->stream, fd, STREAM_READ, server->ssl_ctx);
}

static void server_ssl_descriptor_callback(reactor_event *event)
{
  server *server = event->state;
  int fd;

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
  if (request->state != SERVER_REQUEST_ABORT)
  {
    request->state = SERVER_REQUEST_ABORT;
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

void server_respond(server_request *request, data status, data type, data body)
{
  if (request->state == SERVER_REQUEST_HANDLING)
  {
    http_write_response(&request->stream, status, server_date(), type, body);
    if (request->event_triggered)
      request->state = SERVER_REQUEST_READ;
    else
    {
      request->state = SERVER_REQUEST_NEED_MORE_DATA;
      stream_flush(&request->stream);
      server_request_update(request);
    }
  }
  server_release(request);
}

void server_ok(server_request *request, data type, data body)
{
  server_respond(request, data_string("200 OK"), type, body);
}

void server_not_found(server_request *request)
{
  server_respond(request, data_string("404 Not Found"), data_null(), data_null());
}

void server_bad_request(server_request *request)
{
  if (request->state == SERVER_REQUEST_HANDLING)
  {
    http_write_response(&request->stream, data_string("400 Bad Request"), server_date(), data_null(), data_null());
    stream_flush(&request->stream);
    server_close(request);
  }
  server_release(request);
}
