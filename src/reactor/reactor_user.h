#ifndef REACTOR_REACTOR_USER_H_INCLUDED
#define REACTOR_REACTOR_USER_H_INCLUDED

typedef reactor_status      reactor_user_callback(reactor_event *);
typedef struct reactor_user reactor_user;

struct reactor_user
{
  reactor_user_callback *callback;
  void                  *state;
};

reactor_user   reactor_user_default;
void           reactor_user_construct(reactor_user *, reactor_user_callback *, void *);
reactor_status reactor_user_dispatch(reactor_user *, int, uintptr_t);

#endif /* REACTOR_REACTOR_USER_H_INCLUDED */
