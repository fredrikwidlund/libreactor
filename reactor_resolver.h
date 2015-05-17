#ifndef REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_RESOLVER_H_INCLUDED

typedef struct reactor_resolver reactor_resolver;

struct reactor_resolver
{
  reactor_call        user;
  reactor            *reactor;
  reactor_signal      signal;
};

reactor_resolver *reactor_resolver_new(reactor *, reactor_handler *, void *);
int               reactor_resolver_construct(reactor *, reactor_resolver *, reactor_handler *, void *);
int               reactor_resolver_destruct(reactor_resolver *);
int               reactor_resolver_delete(reactor_resolver *);
void              reactor_resolver_handler(reactor_event *);
int               reactor_resolver_lookup(reactor_resolver *, char *, char *);

#endif /* REACTOR_RESOLVER_H_INCLUDED */

