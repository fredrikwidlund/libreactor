#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <err.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "buffer.h"
#include "vector.h"
#include "reactor.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_resolver.h"

reactor_resolver *reactor_resolver_new(reactor *r, struct gaicb *list[], int nitems, void *o, reactor_handler *h, void *data)
{
  reactor_resolver *s;
  int e;
  
  s = malloc(sizeof *s);
  if (!s)
    return NULL;
  
  e = reactor_resolver_construct(r, s, list, nitems, o, h, data);
  if (e == -1)
    {
      reactor_resolver_delete(s);
      return NULL;
    }

  return s;
}

reactor_resolver *reactor_resolver_new_simple(reactor *r, char *name, char *service, void *o, reactor_handler *h, void *data)
{
  struct addrinfo hints = {.ai_family = AF_INET, .ai_socktype = SOCK_STREAM};
  
  return reactor_resolver_new(r, (struct gaicb *[]){
      (struct gaicb[]){{.ar_name = name, .ar_service = service, .ar_request = &hints}}}, 1, o, h, data);
}

int reactor_resolver_construct(reactor *r, reactor_resolver *s, struct gaicb *list[], int nitems, void *o, reactor_handler *h, void *data)
{
  struct sigevent sigev;
  int e;

  *s = (reactor_resolver) {.user = {.object = o, .handler = h, .data = data}, .reactor = r,
			   .signal = {.object = s, .handler = reactor_resolver_handler, .data = NULL},
			   .list = reactor_resolver_copy_gaicb(list, nitems), .nitems = nitems};
  if (!s->list)
    {
      reactor_resolver_destruct(s);
      return -1;
    }
  
  s->dispatcher = reactor_signal_dispatcher_new(r);
  if (!s->dispatcher)
    {
      reactor_resolver_destruct(s);
      return -1;
    }
      
  sigev = (struct sigevent) {.sigev_notify = SIGEV_SIGNAL, .sigev_signo = REACTOR_SIGNAL_DISPATCHER_SIGNO,
			     .sigev_value.sival_ptr = &s->signal};
  e = getaddrinfo_a(GAI_NOWAIT, s->list, s->nitems, &sigev);
  if (e == -1)
    {
      reactor_resolver_destruct(s);
      return -1;
    }
  
  return 0;
}

struct gaicb **reactor_resolver_copy_gaicb(struct gaicb *list[], int nitems)
{
  struct gaicb *gaicb = calloc(nitems, sizeof *gaicb);
  struct gaicb **copy = calloc(nitems, sizeof *copy);
  int i;

  if (!gaicb || !copy)
    {
      free(gaicb);
      free(copy);
      return NULL;
    }

  for (i = 0; i < nitems; i ++)
    {
      copy[i] = &gaicb[i];
      if (list[i]->ar_name)
	copy[i]->ar_name = strdup(list[i]->ar_name);
      if (list[i]->ar_service)
	copy[i]->ar_service = strdup(list[i]->ar_service);
      if (list[i]->ar_request)
	{
	  copy[i]->ar_request = malloc(sizeof(struct addrinfo));
	  if (copy[i]->ar_request)
	    memcpy((struct addrinfo *) copy[i]->ar_request, list[i]->ar_request, sizeof(struct addrinfo));
	}
    }
  
  return copy;
}

int reactor_resolver_destruct(reactor_resolver *s)
{
  int e, i;
  
  if (s->dispatcher)
    {
      for (i = 0; i < s->nitems; i ++)
	{
	  e = gai_cancel(s->list[i]);
	  if (e != EAI_ALLDONE)
	    return -1;
	}
      
      e = reactor_signal_dispatcher_delete(s->dispatcher);
      if (e == -1)
	return -1;
      s->dispatcher = NULL;
    }
  
  if (s->list)
    {
      for (i = 0; i < s->nitems; i ++)
	{
	  if (s->list[i]->ar_name)
	    free((char *) s->list[i]->ar_name);
	  if (s->list[i]->ar_service)
	    free((char *) s->list[i]->ar_service);
	  if (s->list[i]->ar_request)
	    free((struct sockaddr *) s->list[i]->ar_request);
	  if (s->list[i]->ar_result)
	    freeaddrinfo(s->list[i]->ar_result);
	}
      free(s->list[0]);
      free(s->list);
      s->list = NULL;
    }
  
  return 0;
}

int reactor_resolver_delete(reactor_resolver *s)
{
  int e;
  
  e = reactor_resolver_destruct(s);
  if (e == -1)
    return -1;

  free(s);
  return 0;
}

void reactor_resolver_handler(reactor_event *e)
{
  reactor_resolver *s = e->receiver->object;

  reactor_dispatch_call(s, &s->user, REACTOR_RESOLVER_RESULT, NULL);
}
