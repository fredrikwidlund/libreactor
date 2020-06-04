#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/socket.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int debug_read;
extern int debug_inotify_init1;

static void touch(char *dir, char *file)
{
  char path[PATH_MAX];
  FILE *f;

  snprintf(path, sizeof path, "%s/%s", dir, file);
  f = fopen(path, "w");
  fclose(f);
}

static void cleanup(char *dir, char *file)
{
  char path[PATH_MAX];

  snprintf(path, sizeof path, "%s/%s", dir, file ? file : "");
  remove(path);
}

static core_status callback_abort(core_event *event)
{
  notify *notify = event->state;

  notify_destruct(notify);
  return CORE_ABORT;
}

static core_status callback_ok(core_event *event)
{
  notify *notify = event->state;

  notify_destruct(notify);
  return CORE_OK;
}

static void basic()
{
  notify notify;
  char template[PATH_MAX] = "/tmp/notifyXXXXXX";
  char *path = mkdtemp(template);

  assert_true(path);
  core_construct(NULL);

  // watch on already open notifier
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, path, IN_CLOSE_WRITE);
  notify_watch(&notify, path, IN_CLOSE_WRITE);
  assert_false(notify_valid(&notify));
  core_loop(NULL);

  // notify on close write
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, path, IN_CLOSE_WRITE);
  touch(path, "test");
  core_loop(NULL);

  // notify on close write
  notify_construct(&notify, callback_ok, &notify);
  notify_watch(&notify, path, IN_CLOSE_WRITE);
  touch(path, "test");
  core_loop(NULL);

  // read error when notify
  assert_true(path);
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, path, IN_CLOSE_WRITE);
  touch(path, "test");
  debug_read = ENOBUFS;
  core_loop(NULL);
  debug_read = 0;

  // notify error
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, "/doesnotexist", IN_CLOSE_WRITE);
  core_loop(NULL);

  // notify error
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, "/doesnotexist", IN_CLOSE_WRITE);
  assert_false(notify_valid(&notify));
  notify_destruct(&notify);
  core_loop(NULL);

  // notify init error
  debug_inotify_init1 = 1;
  notify_construct(&notify, callback_abort, &notify);
  notify_watch(&notify, "/", IN_CLOSE_WRITE);
  assert_false(notify_valid(&notify));
  notify_destruct(&notify);
  core_loop(NULL);
  debug_inotify_init1 = 0;

  core_destruct(NULL);
  cleanup(path, "test");
  cleanup(path, NULL);
}

int main()
{
  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(basic)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
