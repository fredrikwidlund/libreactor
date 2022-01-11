#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

#include <netdb.h>
#include "core.h"
#include "async.h"

enum
{
  RESOLVER_SUCCESS,
  RESOLVER_FAILURE
};

typedef struct resolver resolver;

struct resolver
{
  reactor_handler  handler;
  async            async;
  char            *host;
  char            *service;
  int              family;
  int              type;
  int              protocol;
  int              flags;
  struct addrinfo *addrinfo;
};

void resolver_construct(resolver *, reactor_callback *, void *);
void resolver_destruct(resolver *);
void resolver_lookup(resolver *, char *, char *, int, int, int, int);
void resolver_cancel(resolver *);
void resolver_clear(resolver *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */
