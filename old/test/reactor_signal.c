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
extern int debug_signal_error;

static int event_called = 0;

void event(void *state, int type, void *data)
{
  assert_true(state);
  assert_true(data);
  if (type == REACTOR_SIGNAL_RAISE)
    {
      event_called = 1;
      reactor_signal_close(state);
    }
}

void coverage()
{
  reactor_signal signal;
  sigset_t mask;
    int e;

  reactor_signal_init(&signal, event, &signal);

  reactor_core_construct();

  /* sigprocmask error */
  sigemptyset(&mask);
  debug_signal_error = 1;
  e = reactor_signal_open(&signal, &mask);
  assert_int_equal(e, -1);
  debug_signal_error = 0;

    /* signalfd error */
  debug_signal_error = 2;
  e = reactor_signal_open(&signal, &mask);
  assert_int_equal(e, -1);
  debug_signal_error = 0;

  /* reactor_desc_open error */
  debug_signal_error = 3;
  e = reactor_signal_open(&signal, &mask);
  assert_int_equal(e, -1);
  debug_signal_error = 0;

  /* successful open */
  sigaddset(&mask, SIGUSR1);
  e = reactor_signal_open(&signal, &mask);
  assert_int_equal(e, 0);

  /* attempt to open already open signal */
  e = reactor_signal_open(&signal, &mask);
  assert_int_equal(e, -1);

  /* simulate io error on simulated timer event */
  debug_io_error = 1;
  reactor_signal_event(&signal, REACTOR_DESC_READ, NULL);
  debug_io_error = 0;

  /* send signal */
  e = kill(getpid(), SIGUSR1);
  assert_int_equal(e, 0);

  /* run reactor */
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(event_called);

  /* attempt to close already closed signal */
  reactor_signal_close(&signal);

  /* simulate unknown event type */
  reactor_signal_event(&signal, -1, NULL);

  /* simulate signal event on closed signal */
  reactor_signal_event(&signal, REACTOR_DESC_READ, NULL);

  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
