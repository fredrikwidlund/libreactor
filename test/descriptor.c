#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void test1_callback(reactor_event *event)
{
  descriptor *d = event->state;
  char data[1] = {0};

  switch (event->type)
  {
  case DESCRIPTOR_READ:
    assert(read(descriptor_fd(d), data, 1) == 1);
    assert(read(descriptor_fd(d), data, 1) == 0);
    descriptor_destruct(d);
    break;
  case DESCRIPTOR_WRITE:
    assert(write(descriptor_fd(d), data, 1) == 1);
    descriptor_destruct(d);
    break;
  default:
    assert_true(0);
  }
}

static void test2_callback(reactor_event *event)
{
  descriptor *a = event->state;
  switch (event->type)
  {
  case DESCRIPTOR_READ:
    descriptor_destruct(a);
    free(a);
    break;
  }
}

static void test_descriptor(void **state)
{
  descriptor a, b, d, *c;
  int fd[2];

  (void) state;
  assert_int_equal(pipe(fd), 0);
  reactor_construct();
  descriptor_construct(&d, NULL, NULL);
  descriptor_open(&d, fd[0], 0);
  close(fd[1]);
  reactor_loop_once();
  descriptor_destruct(&d);
  reactor_destruct();

  reactor_construct();
  descriptor_construct(&d, NULL, NULL);
  descriptor_destruct(&d);
  reactor_loop();
  reactor_destruct();

  reactor_construct();
  assert_int_equal(pipe(fd), 0);
  descriptor_construct(&a, test1_callback, &a);
  descriptor_construct(&b, test1_callback, &b);
  descriptor_open(&a, fd[0], DESCRIPTOR_READ);
  descriptor_open(&b, fd[1], DESCRIPTOR_WRITE); 
  assert_int_equal(descriptor_fd(&a), fd[0]);
  assert_int_equal(descriptor_fd(&b), fd[1]);
  reactor_loop();
  descriptor_destruct(&a);
  descriptor_destruct(&b);
  reactor_destruct();

  reactor_construct();
  assert_int_equal(pipe(fd), 0);
  descriptor_construct(&d, NULL, NULL);
  descriptor_open(&d, fd[0], 0);
  descriptor_mask(&d, DESCRIPTOR_READ);
  descriptor_mask(&d, DESCRIPTOR_WRITE);
  descriptor_mask(&d, DESCRIPTOR_LEVEL);
  descriptor_destruct(&d);
  reactor_loop();
  close(fd[1]);
  descriptor_destruct(&d);
  reactor_destruct();

  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
  reactor_construct();
  c = malloc(sizeof *c);
  descriptor_construct(c, test2_callback, c);
  descriptor_open(c, fd[1], 1);
  assert_int_equal(write(fd[0], "", 1), 1);
  reactor_loop();
  reactor_destruct();
  (void) close(fd[1]);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
        cmocka_unit_test(test_descriptor)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
