#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <dynamic.h>

#include "../reactor.h"

static reactor_status reactor_user_default_callback(reactor_event *event __attribute__((unused)))
{
  return REACTOR_OK;
}

reactor_user reactor_user_default =
  {
   .callback = reactor_user_default_callback,
   .state = NULL
  };

void reactor_user_construct(reactor_user *user, reactor_user_callback *callback, void *state)
{
  *user = callback ? (reactor_user) {.callback = callback, .state = state} : reactor_user_default;
                      //  *user = (reactor_user) {.callback = callback ? callback : reactor_user_default_callback, .state = state};
}

reactor_status reactor_user_dispatch(reactor_user *user, int type, uintptr_t data)
{
  return user->callback((reactor_event[]){{.state = user->state, .type = type, .data = data}});
}
