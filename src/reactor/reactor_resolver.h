#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

enum reactor_resolver_state
{
  REACTOR_RESOLVER_STATE_CLOSED = 0x01,
  REACTOR_RESOLVER_STATE_OPEN   = 0x02,
  REACTOR_RESOLVER_STATE_ERROR  = 0x04,
};

enum reactor_resolver_event
{
  REACTOR_RESOLVER_EVENT_RESULT,
  REACTOR_RESOLVER_EVENT_ERROR,
  REACTOR_RESOLVER_EVENT_CLOSE
};

typedef struct reactor_resolver reactor_resolver;
struct reactor_resolver
{
  int                  ref;
  int                  state;
  reactor_user         user;
  char                *node;
  char                *service;
  struct addrinfo     *addrinfo;
  struct addrinfo      hints;
};

void reactor_resolver_open(reactor_resolver *, reactor_user_callback *, void *, char *, char *, struct addrinfo *);
void reactor_resolver_close(reactor_resolver *);
void reactor_resolver_hold(reactor_resolver *);
void reactor_resolver_release(reactor_resolver *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */
