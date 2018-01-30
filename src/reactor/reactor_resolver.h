#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

enum reactor_resolver_event
{
  REACTOR_RESOLVER_EVENT_ERROR,
  REACTOR_RESOLVER_EVENT_DONE,
};

typedef struct reactor_resolver reactor_resolver;
struct reactor_resolver
{
  reactor_user     user;
  char            *node;
  char            *service;
  struct addrinfo  hints;
  struct addrinfo *addrinfo;
};

int  reactor_resolver_open(reactor_resolver *, reactor_user_callback *, void *, char *, char *, int, int, int);
void reactor_resolver_close(reactor_resolver *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */
