#include "reactor_user.h"

void reactor_user_construct(reactor_user *user, reactor_user_callback *callback, void *state)
{
  *user = (reactor_user) {.callback = callback, .state = state};
}

void reactor_user_dispatch(reactor_user *user, int type, void *data)
{
  user->callback(user->state, type, data);
}
