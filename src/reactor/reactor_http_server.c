#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <time.h>
#include <string.h>
#include <netdb.h>
#include <err.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_memory.h"
#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_pool.h"
#include "reactor_core.h"
#include "reactor_timer.h"
#include "reactor_resolver.h"
#include "reactor_stream.h"
#include "reactor_tcp.h"
#include "reactor_http_parser.h"
#include "reactor_http.h"
#include "reactor_http_server.h"

static void reactor_http_server_hold(reactor_http_server *server)
{
  server->ref ++;
}

static void reactor_http_server_release(reactor_http_server *server)
{
  server->ref --;
  if (reactor_unlikely(!server->ref))
    {
      vector_destruct(&server->map);
      if (server->user.callback)
        reactor_user_dispatch(&server->user, REACTOR_HTTP_SERVER_EVENT_CLOSE, server);
    }
}

static void reactor_http_server_error(reactor_http_server *server)
{
  if (server->user.callback)
    reactor_user_dispatch(&server->user, REACTOR_HTTP_SERVER_EVENT_ERROR, server);
}

static void reactor_http_server_date(reactor_http_server *server)
{
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t t;
  struct tm tm;

  (void) time(&t);
  (void) gmtime_r(&t, &tm);
  (void) strftime(server->date, sizeof server->date, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(server->date, days[tm.tm_wday], 3);
  memcpy(server->date + 8, months[tm.tm_mon], 3);
}

static void reactor_http_server_request(reactor_http_server_session *session, reactor_http_request *request)
{
  reactor_http_server_map *map;
  size_t i;

  map = vector_data(&session->server->map);
  for (i = 0; i < vector_size(&session->server->map); i ++)
    {
      if ((reactor_memory_empty(map[i].method) || reactor_memory_equal(map[i].method, request->method)) &&
          (reactor_memory_empty(map[i].path) || reactor_memory_equal(map[i].path, request->path)))
        {
          reactor_user_dispatch(&map[i].user, REACTOR_HTTP_SERVER_EVENT_REQUEST,
                                (reactor_http_server_context[]){{.session = session, .request = request}});
          return;
        }
    }
}

static void reactor_http_server_event_http(void *state, int type, void *data)
{
  reactor_http_server_session *session = state;
  reactor_http_request *request;

  switch (type)
    {
    case REACTOR_HTTP_EVENT_HANGUP:
    case REACTOR_HTTP_EVENT_ERROR:
      reactor_http_close(&session->http);
      break;
    case REACTOR_HTTP_EVENT_CLOSE:
      reactor_http_server_release(session->server);
      free(session);
      break;
    case REACTOR_HTTP_EVENT_REQUEST:
      request = data;
      reactor_http_server_request(session, request);
      break;
    }
}

static void reactor_http_server_event_tcp(void *state, int type, void *data)
{
  reactor_http_server *server = state;
  reactor_http_server_session *session;
  int s;

  switch (type)
    {
    case REACTOR_TCP_EVENT_ERROR:
      reactor_http_server_error(server);
      break;
    case REACTOR_TCP_EVENT_ACCEPT:
      s = *(int *) data;
      session = malloc(sizeof *session);
      session->server = server;
      reactor_http_server_hold(server);
      reactor_http_open(&session->http, reactor_http_server_event_http, session, s, REACTOR_HTTP_FLAG_SERVER);
      break;
  }
}

static void reactor_http_server_event_timer(void *state, int type, void *data)
{
  reactor_http_server *server = state;

  (void) data;
  switch (type)
    {
    case REACTOR_TIMER_EVENT_ERROR:
      reactor_http_server_error(server);
      break;
    case REACTOR_TIMER_EVENT_CALL:
      reactor_http_server_date(server);
      break;
    }
}

void reactor_http_server_open(reactor_http_server *server, reactor_user_callback *callback, void *state,
                              char *host, char *port)
{
  server->ref = 0;
  server->state = REACTOR_HTTP_SERVER_STATE_OPEN;
  vector_construct(&server->map, sizeof(reactor_http_server_map));
  reactor_user_construct(&server->user, callback, state);
  server->name = "*";
  reactor_http_server_date(server);
  reactor_http_server_hold(server);
  reactor_timer_open(&server->timer, reactor_http_server_event_timer, server, 1000000000, 1000000000);
  reactor_tcp_open(&server->tcp, reactor_http_server_event_tcp, server, host, port, REACTOR_HTTP_FLAG_SERVER);
}

void reactor_http_server_close(reactor_http_server *server)
{
  (void) server;
}

void reactor_http_server_route(reactor_http_server *server, reactor_user_callback *callback, void *state,
                               char *method, char *path)
{
  vector_push_back(&server->map, (reactor_http_server_map[]) {{
        .user = {.callback = callback, .state = state},
        .method = reactor_memory_str(method),
        .path = reactor_memory_str(path)}});
}

void reactor_http_server_respond_mime(reactor_http_server_session *session, char *type, void *body, size_t size)
{
  reactor_http_write_response(&session->http, (reactor_http_response[]){{
        .version = 1,
        .status = 200,
        .reason = reactor_memory_str("OK"),
        .header_count = 3,
        .headers = (reactor_http_header[]){
          {.name = reactor_memory_str("Server"), .value = reactor_memory_str(session->server->name)},
          {.name = reactor_memory_str("Date"), .value = reactor_memory_str(session->server->date)},
          {.name = reactor_memory_str("Content-Type"), .value = reactor_memory_str(type)}
        }, reactor_memory_ref(body, size)}});
}

void reactor_http_server_respond_text(reactor_http_server_session *session, char *text)
{
  reactor_http_server_respond_mime(session, "text/plain", text, strlen(text));
}
