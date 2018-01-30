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

extern int mock_epoll_create_failure;
extern int mock_socketpair_failure;
extern int mock_epoll_wait_failure;

void construct()
{
  int e;

  // construct/destruct
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();

  // destruct already destructed
  reactor_core_destruct();

  // construct with pool error;
  mock_socketpair_failure = 1;
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_ERROR);
  mock_socketpair_failure = 0;

  // construct with epoll_create error;
  mock_epoll_create_failure = 1;
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_ERROR);
  mock_epoll_create_failure = 0;
  reactor_core_destruct();

}

void run()
{
  reactor_descriptor d;
  int e;

  // run empty core
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();

  // run with epoll_wait error
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  e = reactor_descriptor_open(&d, NULL, NULL, 0, 0);
  assert_int_equal(e, REACTOR_OK);
  mock_epoll_wait_failure = 1;
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_ERROR);
  mock_epoll_wait_failure = 0;
  reactor_core_destruct();
}

int add_event(void *state, int type, void *data)
{
  reactor_descriptor *d = state;

  (void) type;
  (void) data;
  reactor_descriptor_close(&d[0]);
  reactor_descriptor_close(&d[1]);
  return REACTOR_ABORT;
}

void add()
{
  reactor_descriptor d[2];
  int e, fd[2];

  // add broken descriptor
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  e = reactor_descriptor_open(&d[0], NULL, NULL, -1, 0);
  assert_int_equal(e, REACTOR_ERROR);
  reactor_core_destruct();

  // add close queued in loop
  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  fd[0] = dup(0);
  fd[1] = dup(0);
  e = reactor_descriptor_open(&d[0], add_event, d, fd[0], EPOLLOUT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_descriptor_open(&d[1], add_event, d, fd[1], EPOLLOUT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();
  
}

int pool_event(void *state, int type, void *data)
{
  (void) state;
  (void) data;
  if (type == REACTOR_POOL_EVENT_CALL)
    sleep(1);
  return REACTOR_OK;
}

void pool()
{
  int e;

  e = reactor_core_construct();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_enqueue(pool_event, NULL);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);
  reactor_core_destruct();
}

/*

void reader_callback(void *state, int type, void *data)
{
  int e, *fd = state;
  char buffer[256];
  ssize_t n;

  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      assert_true(data == NULL);
      n = recv(*fd, buffer, sizeof buffer, 0);
      if (n > 0)
        {
          assert_int_equal(n, 5);
          assert_string_equal(buffer, "test");
        }
      e = close(*fd);
      assert_int_equal(e, 0);
      reactor_core_fd_deregister(*fd);
      break;
    case REACTOR_CORE_FD_EVENT_WRITE:
      break;
    case REACTOR_CORE_FD_EVENT_HANGUP:
      e = close(*fd);
      assert_int_equal(e, 0);
      reactor_core_fd_deregister(*fd);
      break;
    }
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

  // poll on fd read
  reactor_core_fd_register(fd[0], reader_callback, &fd[0], 0);
  reactor_core_fd_clear(fd[0], REACTOR_CORE_FD_MASK_READ);
  reactor_core_fd_set(fd[0], REACTOR_CORE_FD_MASK_READ | REACTOR_CORE_FD_MASK_WRITE);
  n = send(fd[1], "test", 5, 0);
  assert_int_equal(n, 5);

  // dummy register low fd on non empty core
  reactor_core_fd_register(fd0, NULL, NULL, REACTOR_CORE_FD_MASK_READ);
  reactor_core_fd_deregister(fd0);
  e = close(fd0);
  assert_int_equal(e, 0);

  // process
  e = reactor_core_run();
  assert_int_equal(e, 0);

  e = close(fd[1]);
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

struct reader_large_state
{
  int    fd[2];
  size_t count;
};

void reader_large_event(void *state, int type, void *data)
{
  struct reader_large_state *reader = state;
  int e;
  char block[65536] = {0};
  ssize_t n;

  (void) data;
  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      n = read(reader->fd[0], block, sizeof block);
      if (n == 0)
        break;
      assert_int_equal(n, sizeof block);
      if (reader->count)
        {
          n = write(reader->fd[1], block, sizeof block);
          assert_int_equal(n, sizeof block);
          reader->count --;
        }
      else
        {
          e = close(reader->fd[1]);
          assert_int_equal(e, 0);
        }
      break;
    case REACTOR_CORE_FD_EVENT_HANGUP:
      e = close(reader->fd[0]);
      assert_int_equal(e, 0);
      reactor_core_fd_deregister(reader->fd[0]);
      break;
    }
}

void reader_large()
{
  struct reader_large_state state;
  int e;
  char block[65536] = {0};
  ssize_t n;

  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, state.fd);
  assert_int_equal(e, 0);
  state.count = 128;
  reactor_core_fd_register(state.fd[0], reader_large_event, &state, REACTOR_CORE_FD_MASK_READ);
  n = write(state.fd[1], block, sizeof block);
  assert_int_equal(n, sizeof block);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void reader_hangup()
{
  int fd[2], e;

  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], reader_callback, &fd[0], REACTOR_CORE_FD_MASK_READ);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void pipe_hangup()
{
  int fd[2], e;

  reactor_core_construct();
  e = pipe(fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], reader_callback, &fd[0], REACTOR_CORE_FD_MASK_READ);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void writer_hangup_event(void *state, int type, void *data)
{
  int e, *fd = state;
  char buffer[256];
  ssize_t n;

  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      assert_true(data == NULL);
      n = recv(*fd, buffer, sizeof buffer, 0);
      assert_int_equal(n, 0);
      break;
    case REACTOR_CORE_FD_EVENT_HANGUP:
      e = close(*fd);
      assert_int_equal(e, 0);
      reactor_core_fd_deregister(*fd);
      break;
    }
}

void writer_hangup()
{
  int fd[2], e;

  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], writer_hangup_event, &fd[0], REACTOR_CORE_FD_MASK_READ);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

void poll_failure()
{
  int e;

  // mock poll failure
  mock_poll_failure = 1;
  reactor_core_construct();
  reactor_core_fd_register(0, NULL, 0, REACTOR_CORE_FD_MASK_READ);
  e = reactor_core_run();
  assert_int_equal(e, -1);
  reactor_core_destruct();
  mock_poll_failure = 0;
}

void fd_event(void *state, int type, void *data)
{
  char buffer[1024];
  int *fd = state;
  ssize_t n;

  assert_true(data == NULL);
  switch (type)
    {
    case REACTOR_CORE_FD_EVENT_READ:
      n = read(*fd, buffer, sizeof buffer);
      assert_int_equal(n, 5);
      close(*fd);
      reactor_core_fd_deregister(*fd);
      break;
    case REACTOR_CORE_FD_EVENT_WRITE:
      close(*fd);
      reactor_core_fd_deregister(*fd);
      break;
    case REACTOR_CORE_FD_EVENT_ERROR:
    case REACTOR_CORE_FD_EVENT_HANGUP:
      close(*fd);
      reactor_core_fd_deregister(*fd);
      break;
    }
}

void fd()
{
  int e, fd[2048];
  ssize_t i, n;

  // read
  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], fd_event, &fd[0], REACTOR_CORE_FD_MASK_READ);
  n = write(fd[1], "test", 5);
  assert_int_equal(n, 5);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  reactor_core_destruct();

  // many reads
  reactor_core_construct();
  for (i = 0; i < 512; i += 2)
    {
      e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, &fd[i]);
      assert_int_equal(e, 0);
      reactor_core_fd_register(fd[i], fd_event, &fd[i], REACTOR_CORE_FD_MASK_READ);
      n = write(fd[i + 1], "test", 5);
      assert_int_equal(n, 5);
    }
  e = reactor_core_run();
  assert_int_equal(e, 0);
  for (i = 0; i < 512; i += 2)
    {
      e = close(fd[i + 1]);
      assert_int_equal(e, 0);
    }
  reactor_core_destruct();

  // write
  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], fd_event, &fd[0], REACTOR_CORE_FD_MASK_WRITE);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  reactor_core_destruct();

  // read & write
  reactor_core_construct();
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNIX, fd);
  assert_int_equal(e, 0);
  reactor_core_fd_register(fd[0], fd_event, &fd[0], REACTOR_CORE_FD_MASK_WRITE | REACTOR_CORE_FD_MASK_READ);
  n = write(fd[1], "test", 5);
  assert_int_equal(n, 5);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  e = close(fd[1]);
  assert_int_equal(e, 0);
  reactor_core_destruct();

  // open closed fd
  reactor_core_construct();
  fd[0] = 1000;
  reactor_core_fd_register(fd[0], fd_event, &fd[0], REACTOR_CORE_FD_MASK_READ);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  reactor_core_destruct();
}

*/

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(construct),
    cmocka_unit_test(run),
    cmocka_unit_test(add),
    cmocka_unit_test(pool),
    /*
    cmocka_unit_test(reader),
    cmocka_unit_test(reader_large),
    cmocka_unit_test(reader_hangup),
    cmocka_unit_test(pipe_hangup),
    cmocka_unit_test(poll_failure),
    cmocka_unit_test(writer_hangup),
    cmocka_unit_test(fd)
    */
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
