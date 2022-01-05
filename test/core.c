#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>

#include <cmocka.h>

#include "reactor.h"

extern int debug_epoll_wait;

struct handler_state
{
  reactor_handler h;
  int fd[2];
};

static void handler_callback(reactor_event *event)
{
  struct handler_state *state = event->state;
  struct epoll_event *e = (struct epoll_event *) event->data;
  char data[1] = {0};

  assert_true(event->type == REACTOR_EPOLL_EVENT);
  if (e->events & EPOLLIN)
  {
    assert_int_equal(read(state->fd[0], data, 1), 1);
    reactor_delete(&state->h, state->fd[0]);
  }
  if (e->events & EPOLLOUT)
  {
    assert_int_equal(write(state->fd[1], data, 1), 1);
    reactor_delete(&state->h, state->fd[1]);
  }
}

static void handler(void **arg)
{
  struct handler_state state;

  (void) arg;
  reactor_handler_construct(&state.h, NULL, NULL);
  reactor_dispatch(&state.h, 0, 0);
  reactor_handler_destruct(&state.h);
  
  reactor_construct();
  assert_true(pipe(state.fd) == 0);
  reactor_handler_construct(&state.h, handler_callback, &state);
  reactor_add(&state.h, state.fd[0], EPOLLIN);
  reactor_add(&state.h, state.fd[1], EPOLLOUT);
  reactor_loop();
  reactor_destruct();
  assert_int_equal(close(state.fd[0]), 0);
  assert_int_equal(close(state.fd[1]), 0);
}

static void basic(void **state)
{
  (void) state;
  reactor_construct();
  assert_true(reactor_now() > 0);
  assert_true(reactor_now() > 0);
  reactor_add(NULL, 0, 0);
  reactor_modify(NULL, 0, 0);
  reactor_delete(NULL, 0);
  reactor_abort();
  reactor_loop();
  reactor_destruct();
}

static void test_signal(void **state)
{
  (void) state;

  /* test termination due to epoll interrupt */
  debug_epoll_wait = 1;
  reactor_construct();
  reactor_loop_once();
  reactor_destruct();
  debug_epoll_wait = 0;

  /* test termination due to signal */
  reactor_construct();
  raise(SIGINT);
  reactor_loop();
  reactor_destruct();

  /* test termination of invalid reactor */
  reactor_construct();
  reactor_destruct();
  expect_assert_failure(reactor_loop_once());
}

struct race_state
{
  reactor_handler h;
  int a[2];
  int b[2];
};

static void race_callback(reactor_event *event)
{
  struct race_state *state = event->state;
  struct epoll_event *e = (struct epoll_event *) event->data;

  assert_true(event->type == REACTOR_EPOLL_EVENT);
  if (e->events & EPOLLIN)
  {
    assert_true(state->a[0] >= 0 && state->b[0] >= 0);
    reactor_delete(&state->h, state->a[0]);
    reactor_delete(&state->h, state->b[0]);
    close(state->a[0]);
    close(state->b[0]);
    state->a[0] = -1;
    state->b[0] = -1;
  }
}

static void race(void **arg)
{
  struct race_state state;

  (void) arg;
  reactor_construct();
  assert_true(pipe(state.a) == 0);
  assert_true(pipe(state.b) == 0);
  reactor_handler_construct(&state.h, race_callback, &state);
  reactor_add(&state.h, state.a[0], EPOLLIN);
  reactor_add(&state.h, state.b[0], EPOLLIN);
  assert_int_equal(write(state.a[1], "", 1), 1);
  assert_int_equal(write(state.b[1], "", 1), 1);
  reactor_loop();
  reactor_destruct();
  assert_int_equal(close(state.a[1]), 0);
  assert_int_equal(close(state.b[1]), 0);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(handler),
          cmocka_unit_test(basic),
          cmocka_unit_test(test_signal),
          cmocka_unit_test(race)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
