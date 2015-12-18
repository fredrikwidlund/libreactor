#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <setjmp.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor_core.h"

extern int debug_io_error;
extern int debug_epoll_error;

typedef struct env env;
struct env
{
  reactor_desc read;
  int          read_called;
  reactor_desc write;
  int          write_called;
};

void write_event(void *state, int type, void *data)
{
  env *env;
  ssize_t n;

  env = state;
  assert_true(data == &env->write);
  if (type & REACTOR_DESC_WRITE)
    {
      env->write_called ++;
      n = write(reactor_desc_fd(&env->write), ".", 1);
      assert_int_equal(n, 1);
      reactor_desc_close(&env->write);
    }
}

void read_event(void *state, int type, void *data)
{
  env *env;
  ssize_t n;
  char c;

  env = state;
  assert_true(data == &env->read);
  if (type & REACTOR_DESC_READ)
    {
      env->read_called ++;
      do
        n = read(reactor_desc_fd(&env->read), &c, 1);
      while (n == 1);
      reactor_desc_close(&env->read);
    }
}

void coverage()
{
  env env;
  int e, fds[2];

  bzero(&env, sizeof env);

  reactor_core_construct();
  e = pipe(fds);
  assert_int_equal(e, 0);

  reactor_desc_init(&env.write, write_event, &env);
  reactor_desc_init(&env.read, read_event, &env);

  /* fcntl error */
  debug_io_error = 1;
  e = reactor_desc_open(&env.read, fds[0]);
  assert_int_equal(e, -1);
  debug_io_error = 0;

  /* epoll_ctl error */
  debug_epoll_error = 1;
  e = reactor_desc_open(&env.read, fds[0]);
  assert_int_equal(e, -1);
  debug_epoll_error = 0;

  /* pipe transaction */
  e = reactor_desc_open(&env.write, fds[1]);
  assert_int_equal(e, 0);
  reactor_desc_events(&env.write, REACTOR_DESC_WRITE);
  e = reactor_desc_open(&env.read, fds[0]);
  assert_int_equal(e, 0);

  /* update when events are not changed */
  reactor_desc_events(&env.read, REACTOR_DESC_READ);
  reactor_desc_events(&env.read, REACTOR_DESC_READ);

  /* run reactor */
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(env.read_called, 1);
  assert_int_equal(env.write_called, 1);

  /* close on already closed fd */
  reactor_desc_close(&env.read);

  /* update events on already closed desc */
  reactor_desc_events(&env.read, 0);

  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
