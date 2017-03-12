#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

struct core_state
{
  reactor_resolver resolver;
  int              result;
  int              error;
  int              close;
};

void core_event(void *state, int type, void *data)
{
  struct core_state *core_state = state;
  struct addrinfo *addrinfo;

  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_RESULT:
      addrinfo = data;
      core_state->result = addrinfo != NULL;
      break;
    case REACTOR_RESOLVER_EVENT_ERROR:
      core_state->error = 1;
      break;
    case REACTOR_RESOLVER_EVENT_CLOSE:
      core_state->close = 1;
      break;
    }
}

void core()
{
  struct core_state state;
  int e;

  state = (struct core_state) {0};
  reactor_core_construct();
  reactor_resolver_open(&state.resolver, core_event, &state, "127.0.0.1", "80", NULL);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(state.result, 1);
  assert_int_equal(state.error, 0);
  assert_int_equal(state.close, 1);
  reactor_core_destruct();

  state = (struct core_state) {0};
  reactor_core_construct();
  reactor_resolver_open(&state.resolver, core_event, &state, "x", "y", NULL);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(state.result, 0);
  assert_int_equal(state.error, 0);
  assert_int_equal(state.close, 1);
  reactor_core_destruct();

  state = (struct core_state) {0};
  reactor_core_construct();
  reactor_resolver_open(&state.resolver, core_event, &state, "localhost", "http", NULL);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(state.result, 1);
  assert_int_equal(state.error, 0);
  assert_int_equal(state.close, 1);
  reactor_core_destruct();

  state = (struct core_state) {0};
  reactor_core_construct();
  reactor_resolver_open(&state.resolver, core_event, &state, "127.0.0.1", "80",
                        (struct addrinfo[]){{.ai_flags = 0xffff}});
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(state.result, 0);
  assert_int_equal(state.error, 1);
  assert_int_equal(state.close, 1);
  reactor_core_destruct();

  reactor_resolver_close(&state.resolver);
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}

