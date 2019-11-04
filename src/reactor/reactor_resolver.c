#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <dynamic.h>

#include "../reactor.h"

typedef struct reactor_resolver_job reactor_resolver_job;
struct reactor_resolver_job
{
  reactor_user      user;
  int               active;
  char             *node;
  char             *service;
  int               family;
  int               socktype;
  int               flags;
  struct addrinfo  *addrinfo;
};

typedef struct reactor_resolver reactor_resolver;
struct reactor_resolver
{
  int               ref;
  list              jobs;
};

static __thread reactor_resolver resolver = {0};

static void reactor_resolver_release_job(void *item)
{
  reactor_resolver_job *job = item;

  freeaddrinfo(job->addrinfo);
  free(job->node);
  free(job->service);
}

static reactor_status reactor_resolver_handler(reactor_event *event)
{
  reactor_resolver_job *job = event->state;
  struct addrinfo hints;

  if (event->type == REACTOR_POOL_EVENT_CALL)
    {
      hints = (struct addrinfo) {.ai_family = job->family, .ai_socktype = job->socktype, .ai_flags = job->flags};
      (void) getaddrinfo(job->node, job->service, &hints, &job->addrinfo);
    }
  else
    {
      if (job->active)
        (void) reactor_user_dispatch(&job->user, REACTOR_RESOLVER_EVENT_RESPONSE, (uintptr_t) job->addrinfo);
      list_erase(job, reactor_resolver_release_job);
    }

  return REACTOR_OK;
}

void reactor_resolver_construct(void)
{
  if (!resolver.ref)
    list_construct(&resolver.jobs);

  resolver.ref ++;
}

void reactor_resolver_destruct(void)
{
  resolver.ref --;
  if (!resolver.ref)
    list_destruct(&resolver.jobs, reactor_resolver_release_job);
}

reactor_id reactor_resolver_request(reactor_user_callback *callback, void *state, char *node, char *service,
                                    int family, int socktype, int flags)
{
  reactor_resolver_job *job;

  if (!resolver.ref)
    return 0;

  job = list_push_back(&resolver.jobs, NULL, sizeof *job);
  reactor_user_construct(&job->user, callback, state);
  job->active = 1;
  job->node = strdup(node);
  job->service = strdup(service);
  job->family = family;
  job->socktype = socktype;
  job->flags = flags;
  job->addrinfo = NULL;
  reactor_pool_dispatch(reactor_resolver_handler, job);

  return (reactor_id) job;
}

void reactor_resolver_cancel(reactor_id id)
{
  reactor_resolver_job *job = (reactor_resolver_job *) id;

  if (job)
    job->active = 0;
}
