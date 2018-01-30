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
#include <sys/epoll.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static int core_event(void *state, int type, void *data)
{
  reactor_descriptor *d = state;

  (void) type;
  (void) data;
  reactor_descriptor_close(d);
  return REACTOR_ABORT;
}

void core()
{
  reactor_descriptor d;
  int e, fd;

  // open descriptor
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  fd = dup(0);
  e = reactor_descriptor_open(&d, core_event, &d, fd, EPOLLOUT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();

  // close already closed
  reactor_descriptor_close(&d);

  // clear closed
  e = reactor_descriptor_clear(&d);
  assert_int_equal(e, REACTOR_ERROR);
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
