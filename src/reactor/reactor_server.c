#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_server_process(reactor_server_session *session, reactor_http_request *request)
{
  reactor_user *user;
  int e;

  if (!(session->flags & REACTOR_SERVER_SESSION_READY))
    {
      session->flags |= REACTOR_SERVER_SESSION_CLOSE;
      return REACTOR_OK;
    }

  session->flags = 0;
  session->request = request;

  if (reactor_http_headers_lookup(&session->request->headers, reactor_vector_string("Origin")).size)
    session->flags |= REACTOR_SERVER_SESSION_CORS;

  if (reactor_http_headers_match(&session->request->headers,
                                 reactor_vector_string("Connection"), reactor_vector_string("Close")))
    session->flags |= REACTOR_SERVER_SESSION_CLOSE;

  list_foreach(&session->server->routes, user)
    {
      e = reactor_user_dispatch(user, REACTOR_SERVER_EVENT_REQUEST, (uintptr_t) session);
      if (e != REACTOR_OK)
        return e;

      if (session->flags & REACTOR_SERVER_SESSION_HANDLED)
       break;
    }

  if (!(session->flags & REACTOR_SERVER_SESSION_HANDLED))
    reactor_server_not_found(session);

  session->request = NULL;
  return REACTOR_OK;
}

static reactor_status reactor_server_session_handler(reactor_event *event)
{
  switch (event->type)
    {
    case REACTOR_HTTP_EVENT_REQUEST:
      return reactor_server_process(event->state, (reactor_http_request *) event->data);
    default:
      reactor_server_close(event->state);
      return REACTOR_ABORT;
    }
}

static reactor_status reactor_server_error(reactor_server *server, char *description)
{
  return reactor_user_dispatch(&server->user, REACTOR_SERVER_EVENT_ERROR, (uintptr_t) description);
}

static void reactor_server_open_session(reactor_server *server, int fd)
{
  reactor_server_session *session;

  session = list_push_back(&server->sessions, NULL, sizeof (reactor_server_session));
  reactor_http_construct(&session->http, reactor_server_session_handler, session);
  reactor_http_set_mode(&session->http, REACTOR_HTTP_MODE_REQUEST);
  session->server = server;
  session->flags = REACTOR_SERVER_SESSION_READY;

  reactor_http_open(&session->http, fd);
}

void reactor_server_close(reactor_server_session *session)
{
  reactor_http_destruct(&session->http);
  if (session->flags & REACTOR_SERVER_SESSION_REGISTERED)
    (void) reactor_user_dispatch(&session->user, REACTOR_SERVER_EVENT_CLOSE, 0);
  list_erase(session, NULL);
}

reactor_status reactor_server_net_handler(reactor_event *event)
{
  reactor_server *server = event->state;

  switch (event->type)
    {
    case REACTOR_NET_EVENT_ACCEPT:
      reactor_server_open_session(server, event->data);
      return REACTOR_OK;
    default:
      return reactor_server_error(server, "unhandled network event");
    }

  return REACTOR_OK;
}

void reactor_server_construct(reactor_server *server, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&server->user, callback, state);
  reactor_net_construct(&server->net, reactor_server_net_handler, server);
  list_construct(&server->routes);
  list_construct(&server->sessions);
}

void reactor_server_destruct(reactor_server *server)
{
  reactor_net_destruct(&server->net);
  list_destruct(&server->routes, NULL);
}

reactor_status reactor_server_open(reactor_server *server, char *node, char *service)
{
  return reactor_net_bind(&server->net, node, service);
}

void reactor_server_route(reactor_server *server, reactor_user_callback *callback, void *state)
{
  reactor_user user;

  reactor_user_construct(&user, callback, state);
  list_push_back(&server->routes, &user, sizeof user);
}

void reactor_server_register(reactor_server_session *session, reactor_user_callback *callback, void *state)
{
  reactor_user_construct(&session->user, callback, state);
  session->flags |= REACTOR_SERVER_SESSION_REGISTERED | REACTOR_SERVER_SESSION_HANDLED;
}

void reactor_server_respond(reactor_server_session *session, reactor_http_response *response)
{
  if (session->flags & REACTOR_SERVER_SESSION_CORS)
    reactor_http_headers_add(&response->headers,
                             reactor_vector_string("Access-Control-Allow-Origin"), reactor_vector_string("*"));

  reactor_http_write_response(&session->http, response);
  reactor_http_flush(&session->http);

  if (session->flags & REACTOR_SERVER_SESSION_CLOSE)
    reactor_http_shutdown(&session->http);
  else
    session->flags |= REACTOR_SERVER_SESSION_READY;

  session->flags |= REACTOR_SERVER_SESSION_HANDLED;
  session->flags &= ~REACTOR_SERVER_SESSION_REGISTERED;
}

void reactor_server_ok(reactor_server_session *session, reactor_vector type, reactor_vector body)
{
  reactor_http_response response;

  reactor_http_create_response(&session->http, &response, 1, 200, reactor_vector_string("OK"), type, body.size, body);
  reactor_server_respond(session, &response);
}

void reactor_server_found(reactor_server_session *session, reactor_vector location)
{
  reactor_http_response response;

  reactor_http_create_response(&session->http, &response, 1, 302, reactor_vector_string("Found"),
                               reactor_vector_empty(), 0, reactor_vector_empty());
  reactor_http_headers_add(&response.headers, reactor_vector_string("Location"), location);
  reactor_server_respond(session, &response);
}

void reactor_server_not_found(reactor_server_session *session)
{
  reactor_http_response response;

  reactor_http_create_response(&session->http, &response, 1, 404, reactor_vector_string("Not Found"),
                               reactor_vector_empty(), 0, reactor_vector_empty());
  reactor_server_respond(session, &response);
}
