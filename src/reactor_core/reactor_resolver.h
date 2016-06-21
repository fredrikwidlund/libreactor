#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

enum reactor_resolver_state
{
  REACTOR_RESOLVER_CLOSED,
  REACTOR_RESOLVER_OPEN,
  REACTOR_RESOLVER_INVALID,
  REACTOR_RESOLVER_CLOSE_WAIT,
};

enum reactor_resolver_resolvers
{
  REACTOR_RESOLVER_ERROR,
  REACTOR_RESOLVER_SIGNAL,
  REACTOR_RESOLVER_CLOSE
};

typedef struct reactor_resolver reactor_resolver;
struct reactor_resolver
{
  int              state;
  reactor_user     user;
  reactor_event    event;
  int              ref;
  char            *stack;
  char            *node;
  char            *service;
  struct addrinfo *hints;
};

void   reactor_resolver_init(reactor_resolver *, reactor_user_callback *, void *);
void   reactor_resolver_open(reactor_resolver *, char *, char *, struct addrinfo *);
void   reactor_resolver_close(reactor_resolver *);
void   reactor_resolver_event(void *, int, void *);
void   reactor_resolver_error(reactor_resolver *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */
