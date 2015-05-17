#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "reactor.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_resolver.h"
#include "reactor_fd.h"
#include "reactor_socket.h"
#include "reactor_client.h"

reactor_client  *reactor_client_new(reactor *r, reactor_handler *h, int type, char *name, char *service, void *user)
{
  reactor_client *c;
  int e;
  
  c = malloc(sizeof *c);
  if (!c)
    return NULL;

  e = reactor_client_construct(r, c, h, type, name, service, user);
  if (e == -1)
    {
      reactor_client_delete(c);
      return NULL;
    }

  return c;
}

int reactor_client_construct(reactor *r, reactor_client *c, reactor_handler *h, int type, char *name, char *service, void *data)
{  
  *c = (reactor_client) {.user= {.handler = h, .data = data}, .reactor = r, .state = REACTOR_CLIENT_STATE_INIT,
			 .type = type, .name = name, .service = service};

  return reactor_client_update(c);
}

int reactor_client_destruct(reactor_client *c)
{
  return 0;
}

int reactor_client_delete(reactor_client *c)
{
  int e;
  
  e = reactor_client_destruct(c);
  if (e == -1)
    return -1;

  free(c);
  return 0;
}

void reactor_client_resolver_handler(reactor_event *event)
{
  reactor_client *c = event->call->data;
  reactor_resolver *s = event->data;
  int e;
  
  if (c->state != REACTOR_CLIENT_STATE_RESOLVING || !s->list[0]->ar_result)
    {
      reactor_client_error(c);
      return;
    }
  c->state = REACTOR_CLIENT_STATE_RESOLVED;
 
  e = reactor_client_connect(c);
  if (e == -1)
    {
      reactor_client_error(c);
      return;
    }
  c->state = REACTOR_CLIENT_STATE_CONNECTING;
}

void reactor_client_socket_handler(reactor_event *event)
{
  reactor_client *c = event->call->data;
  
  if (c->state != REACTOR_CLIENT_STATE_CONNECTING)
    {
      reactor_client_error(c);
      return;
    }
  
  c->state = REACTOR_CLIENT_STATE_CONNECTED;
  reactor_dispatch(&c->user, REACTOR_CLIENT_CONNECTED, NULL);
}

int reactor_client_update(reactor_client *c)
{

  int e;
  switch (c->state)
    {
    case REACTOR_CLIENT_STATE_INIT:
      e = reactor_client_resolve(c);
      if (e == -1)
	return -1;
      c->state = REACTOR_CLIENT_STATE_RESOLVING;
      break;
    default:
      break;
    }
  
  return 0;
}

void reactor_client_error(reactor_client *c)
{
  c->state = REACTOR_CLIENT_STATE_ERROR;
}

int reactor_client_resolve(reactor_client *c)
{
  struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = c->type};
  struct gaicb *list[] = {(struct gaicb[]){{.ar_name = c->name, .ar_service = c->service, .ar_request = &hints}}};

  if (c->state != REACTOR_CLIENT_STATE_INIT)
    return -1;
  
  c->resolver = reactor_resolver_new(c->reactor, reactor_client_resolver_handler, list, 1, c);
  if (!c->resolver)
    return -1;

  return 0;
}

int reactor_client_connect(reactor_client *c)
{
  struct addrinfo *ai = c->resolver->list[0]->ar_result;
  int e;
  
  if (c->state != REACTOR_CLIENT_STATE_RESOLVED)
    return -1;

  c->fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
  if (c->fd == -1)
    return -1;

  e = fcntl(c->fd, F_SETFL, O_NONBLOCK);
  if (e == -1)
    return -1;

  e = connect(c->fd, ai->ai_addr, ai->ai_addrlen);
  if (e == -1 && errno != EINPROGRESS)
    return -1;
  
  c->socket = reactor_socket_new(c->reactor, reactor_client_socket_handler, c->fd, c);
  if (!c->socket)
    return -1;

  e = reactor_socket_write_notify(c->socket);
  if (e == -1)
    return -1;
  
  c->state = REACTOR_CLIENT_STATE_CONNECTING;
  
  return 0;
}
