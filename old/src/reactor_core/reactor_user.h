#ifndef REACTOR_USER_H_INCLUDED
#define REACTOR_USER_H_INCLUDED

typedef void                reactor_user_call(void *, int, void *);
typedef struct reactor_user reactor_user;

struct reactor_user
{
  reactor_user_call *call;
  void              *state;
};

void reactor_user_init(reactor_user *, reactor_user_call *, void *);
void reactor_user_dispatch(reactor_user *, int, void *);

#endif /* REACTOR_USER_H_INCLUDED */
