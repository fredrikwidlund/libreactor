#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int mock_poll_failure;

void core()
{
  int e;

  /* run empty core */
  reactor_core_construct();
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void reader_callback(void *state, int type, void *data)
{
  struct pollfd *pollfd;
  int e, *fd = state;
  char buffer[256];
  ssize_t n;

  pollfd = data;
  assert_true(pollfd->revents & POLLIN);
  assert_int_equal(type, REACTOR_CORE_EVENT_FD);
  assert_int_equal(*fd, pollfd->fd);
  n = recv(pollfd->fd, buffer, sizeof buffer, 0);
  assert_int_equal(n, 5);
  assert_string_equal(buffer, "test");
  e = close(pollfd->fd);
  assert_int_equal(e, 0);
  reactor_core_fd_deregister(pollfd->fd);
}

void reader()
{
  int fd0, fd[2], e;
  ssize_t n;

  reactor_core_construct();

  fd0 = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
  assert_true(fd0 >= 0);

  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);

  /* poll on fd read */
  reactor_core_fd_register(fd[0], reader_callback, &fd[0], POLLIN);
  n = send(fd[1], "test", 5, 0);
  assert_int_equal(n, 5);

  /* dummy register low fd on non empty core */
  reactor_core_fd_register(fd0, NULL, NULL, POLLIN);
  reactor_core_fd_deregister(fd0);
  e = close(fd0);
  assert_int_equal(e, 0);

  /* process */
  e = reactor_core_run();
  assert_int_equal(e, 0);

  e = close(fd[1]);
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void poll_failure()
{
  int e;

  /* simulare poll failure */
  mock_poll_failure = 1;
  reactor_core_construct();
  reactor_core_fd_register(0, NULL, 0, POLLIN);
  e = reactor_core_run();
  assert_int_equal(e, -1);
  reactor_core_destruct();
  mock_poll_failure = 0;
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(reader),
    cmocka_unit_test(poll_failure)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
