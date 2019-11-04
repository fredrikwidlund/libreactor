#ifndef REACTOR_REACTOR_RESOLVER_H_INCLUDED
#define REACTOR_REACTOR_RESOLVER_H_INCLUDED

enum reactor_resolver_event
{
  REACTOR_RESOLVER_EVENT_RESPONSE
};

void       reactor_resolver_construct(void);
void       reactor_resolver_destruct(void);
reactor_id reactor_resolver_request(reactor_user_callback *, void *, char *, char *, int, int, int);
void       reactor_resolver_cancel(reactor_id);

#endif /* REACTOR_REACTOR_RESOLVER_H_INCLUDED */
