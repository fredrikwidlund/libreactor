#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

#define REACTOR_RESOLVER_RESULT 1

typedef struct reactor_resolver reactor_resolver;

struct reactor_resolver
{
  reactor_call               user;
  reactor                   *reactor;
  reactor_signal_dispatcher *dispatcher;
  reactor_call               signal;
  struct gaicb               ar;
};

reactor_resolver *reactor_resolver_new(reactor *, reactor_handler *, char *, char *, void *);
int               reactor_resolver_construct(reactor *, reactor_resolver *, reactor_handler *, char *, char *, void *);
int               reactor_resolver_destruct(reactor_resolver *);
int               reactor_resolver_delete(reactor_resolver *);
void              reactor_resolver_handler(reactor_event *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */

