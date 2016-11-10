#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <err.h>
#include <regex.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "reactor_user.h"
#include "reactor_desc.h"
#include "reactor_core.h"
#include "reactor_stream.h"
#include "reactor_tcp.h"
#include "reactor_http.h"
#include "reactor_timer.h"
#include "reactor_rest.h"

static inline void reactor_rest_dispatch(reactor_rest *rest, int type, void *state)
{
  if (rest->user.callback)
    reactor_user_dispatch(&rest->user, type, state);
}

static inline void reactor_rest_close_final(reactor_rest *rest)
{
  if (rest->state == REACTOR_REST_CLOSE_WAIT &&
      rest->http.state == REACTOR_HTTP_CLOSED &&
      rest->ref == 0)
    {
      vector_clear(&rest->maps);
      rest->state = REACTOR_REST_CLOSED;
      reactor_rest_dispatch(rest, REACTOR_REST_CLOSE, NULL);
    }
}

static void reactor_rest_map_release(void *data)
{
  reactor_rest_map *map;

  map = data;
  free(map->method);
  free(map->path);
  if (map->regex)
    {
      regfree(map->regex);
      free(map->regex);
    }
}

void reactor_rest_init(reactor_rest *rest, reactor_user_callback *callback, void *state)
{
  *rest = (reactor_rest) {.state = REACTOR_REST_CLOSED};
  reactor_user_init(&rest->user, callback, state);
  reactor_http_init(&rest->http, reactor_rest_http_event, rest);
  reactor_timer_init(&rest->timer, reactor_rest_timer_event, rest);
  vector_init(&rest->maps, sizeof(reactor_rest_map));
  vector_release(&rest->maps, reactor_rest_map_release);
}

void reactor_rest_open(reactor_rest *rest, char *node, char *service, int flags)
{
  if (rest->state != REACTOR_REST_CLOSED)
    {
      reactor_rest_error(rest);
      return;
    }

  rest->flags = flags;
  rest->state = REACTOR_REST_OPEN;
  reactor_http_open(&rest->http, node, service, REACTOR_HTTP_SERVER);
  reactor_rest_timer_update(rest);
  reactor_timer_open(&rest->timer, 1000000000, 1000000000);
}

void reactor_rest_error(reactor_rest *rest)
{
  rest->state = REACTOR_REST_INVALID;
  reactor_rest_dispatch(rest, REACTOR_REST_ERROR, NULL);
}

void reactor_rest_close(reactor_rest *rest)
{
  if (rest->state == REACTOR_REST_CLOSED)
    return;

  reactor_http_close(&rest->http);
  reactor_timer_close(&rest->timer);
  rest->state = REACTOR_REST_CLOSE_WAIT;
  reactor_rest_close_final(rest);
}

void reactor_rest_http_event(void *state, int type, void *data)
{
  reactor_rest *rest = state;
  reactor_http_session *session = data;
  reactor_http_message *message = &session->message;
  reactor_rest_request request = {.rest = rest, .session = session};
  reactor_rest_map *map;
  size_t i, nmatch = 32;
  regmatch_t match[nmatch];
  int e;

  switch (type)
    {
    case REACTOR_HTTP_MESSAGE:
      for (i = 0; i < vector_size(&rest->maps); i ++)
        {
          map = vector_at(&rest->maps, i);
          if (!map->method || strcmp(map->method, message->method) == 0)
            switch (map->type)
              {
              case REACTOR_REST_MAP_MATCH:
                if (!map->path || strcmp(map->path, message->path) == 0)
                  {
                    map->handler(map->state, &request);
                    return;
                  }
                break;
              case REACTOR_REST_MAP_REGEX:
                e = regexec(map->regex, message->path, 32, match, 0);
                if (e == 0)
                  {
                    request.match = match;
                    map->handler(map->state, &request);
                    return;
                  }
                break;
              }
        }
      reactor_rest_respond_not_found(&request);
      break;
    case REACTOR_HTTP_ERROR:
      reactor_rest_error(rest);
      break;
    case REACTOR_HTTP_SHUTDOWN:
      reactor_rest_dispatch(rest, REACTOR_REST_SHUTDOWN, NULL);
      break;
    case REACTOR_HTTP_CLOSE:
      reactor_rest_close_final(rest);
      break;
    }
}

void reactor_rest_timer_event(void *state, int type, void *data)
{
  reactor_rest *rest = state;

  (void) data;
  switch (type)
    {
    case REACTOR_TIMER_SIGNAL:
      reactor_rest_timer_update(rest);
      break;
    case REACTOR_TIMER_ERROR:
      reactor_rest_error(rest);
    case REACTOR_TIMER_CLOSE:
      reactor_rest_close_final(rest);
      break;
    }
}

void reactor_rest_timer_update(reactor_rest *rest)
{
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  time_t t;
  struct tm tm;

  (void) time(&t);
  (void) gmtime_r(&t, &tm);
  (void) strftime(rest->date, sizeof rest->date, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(rest->date, days[tm.tm_wday], 3);
  memcpy(rest->date + 8, months[tm.tm_mon], 3);
}

void reactor_rest_add_match(reactor_rest *rest, char *method, char *path, reactor_rest_handler *handler, void *state)
{
  reactor_rest_map map =
    {
      .type = REACTOR_REST_MAP_MATCH,
      .method = method ? strdup(method) : NULL,
      .path = path ? strdup(path) : NULL,
      .handler = handler,
      .state = state
    };

  vector_push_back(&rest->maps, &map);
  if ((method && !map.method) || (path && !map.path))
    reactor_rest_error(rest);
}

void reactor_rest_add_regex(reactor_rest *rest, char *method, char *regex, reactor_rest_handler *handler, void *state)
{
  int e;
  reactor_rest_map map =
    {
      .type = REACTOR_REST_MAP_REGEX,
      .method = method ? strdup(method) : NULL,
      .regex = malloc(sizeof(regex_t)),
      .handler = handler,
      .state = state
    };

  if (map.regex)
    {
      e = regcomp(map.regex, regex, REG_EXTENDED);
      if (e == -1)
        {
          regfree(map.regex);
          free(map.regex);
          map.regex = NULL;
        }
    }

  if (!map.regex)
    {
      free(map.method);
      reactor_rest_error(rest);
      return;
    }

  vector_push_back(&rest->maps, &map);
}

void reactor_rest_respond_empty(reactor_rest_request *request, int status, char *reason)
{
  reactor_http_message message = {
    .type = REACTOR_HTTP_MESSAGE_RESPONSE, .version = 1, .status = status, .reason = reason,
    .header_size = 1, .header = (reactor_http_header[]) {{"Date", request->rest->date}},
    .body = NULL, .body_size = 0
  };
  reactor_http_session_message(request->session, &message);
}

void reactor_rest_respond_body(reactor_rest_request *request, int status, char *reason, char *content_type, void *body, size_t body_size)
{
  reactor_http_message message = {
    .type = REACTOR_HTTP_MESSAGE_RESPONSE, .version = 1, .status = status, .reason = reason,
    .header_size = 2, .header = (reactor_http_header[]) {
      {"Server", "libreactor"},
      {"Date", request->rest->date},
      {"Content-Type", content_type}},
    .body = body, .body_size = body_size,
  };
  reactor_http_session_message(request->session, &message);
}

void reactor_rest_respond_not_found(reactor_rest_request *request)
{
  reactor_rest_respond_empty(request, 404, "Not Found");
}

void reactor_rest_respond_text(reactor_rest_request *request, char *text)
{
  reactor_rest_respond_body(request, 200, "OK", "text/plain", text, strlen(text));
}
