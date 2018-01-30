#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int reactor_resolver_event(void *, int, void *);

static int event(void *state, int type, void *data)
{
  reactor_resolver *r = state;

  switch (type)
    {
    case REACTOR_RESOLVER_EVENT_DONE:
      assert_int_equal(type, REACTOR_RESOLVER_EVENT_DONE);
      assert_true(data != NULL);
      reactor_resolver_close(r);
      return REACTOR_ABORT;
    default:
      return REACTOR_OK;
    }
}

static void core()
{
  reactor_resolver r;

  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_resolver_open(&r, event, &r, "localhost", "http", 0, 0, 0), REACTOR_OK);
  assert_int_equal(reactor_resolver_event(&r, -1, NULL), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();

}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
