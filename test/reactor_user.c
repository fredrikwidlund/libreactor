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

static void callback(void *state, int type, void *data)
{
  assert_true(state != NULL);
  assert_int_equal(type, 42);
  assert_string_equal(data, "test");
}

void core()
{
  reactor_user user;

  reactor_user_construct(&user, callback, &user);
  reactor_user_dispatch(&user, 42, "test");
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
