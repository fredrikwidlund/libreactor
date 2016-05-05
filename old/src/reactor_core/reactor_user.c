#include "reactor_user.h"

void reactor_user_init(reactor_user *user, reactor_user_call *call, void *state)
{
  *user = (reactor_user) {.call = call, .state = state};
}

void reactor_user_dispatch(reactor_user *user, int type, void *data)
{
  user->call(user->state, type, data);
}
