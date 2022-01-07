#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cmocka.h>
#include "reactor.h"

static void callback(reactor_event *event)
{
  notify *n = event->state;

  notify_destruct(n);
  free(n);
}

static void test_notify_basic(void **state)
{
  notify n;

  (void) state;
  /* receive events for file */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  assert_true(notify_watch(&n, "/bin", IN_ALL_EVENTS) > 0);
  assert_true(notify_watch(&n, "/", IN_ALL_EVENTS) > 0);
  assert_int_equal(closedir(opendir("/bin")), 0);
  assert_int_equal(closedir(opendir("/")), 0);
  assert_int_equal(close(open("/bin/sh", O_RDONLY)), 0);
  reactor_loop_once();
  notify_clear(&n);
  notify_destruct(&n);
  reactor_destruct();

  /* watch file that does not exist */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  assert_true(notify_watch(&n, "/invalid/file", IN_ALL_EVENTS) == -1);
  notify_destruct(&n);
  reactor_destruct();
}

static void test_notify_abort(void **state)
{
  notify *n = malloc(sizeof *n);;

  (void) state;
  /* abort while receiving events */
  reactor_construct();
  notify_construct(n, callback, n);
  notify_watch(n, "/", IN_ALL_EVENTS);
  notify_watch(n, "/", IN_ALL_EVENTS);
  assert_int_equal(closedir(opendir("/")), 0);
  reactor_loop_once();
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_notify_basic),
      cmocka_unit_test(test_notify_abort)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
