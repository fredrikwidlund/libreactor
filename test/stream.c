#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/socket.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

/*
static core_status callback_abort(core_event *event)
{
  notify *notify = event->state;

  notify_destruct(notify);
  return CORE_ABORT;
}
*/

static int write_count = 0;
static int read_size =  0;
static int error_count = 0;

static core_status callback_readline(core_event *event)
{
  stream *stream = event->state;
  char *line = "line\n";
  void *base;
  size_t size;

  switch (event->type)
    {
    case STREAM_FLUSH:
      if (write_count)
        {
          stream_write(stream, line, strlen(line));
          stream_flush(stream);
          write_count --;
        }
      else
        stream_destruct(stream);
      return CORE_OK;
    case STREAM_READ:
      stream_readline(stream, &base, &size);
      assert_int_equal(size, strlen(line));
      assert_true(memcmp(base, line, size) == 0);
      stream_destruct(stream);
      return CORE_ABORT;
    default:
      stream_destruct(stream);
      return CORE_ABORT;
    }
}

static core_status callback(core_event *event)
{
  stream *stream = event->state;
  char data[1048576] = {0};
  void *base;
  size_t size;

  switch(event->type)
    {
    case STREAM_FLUSH:
      if (write_count)
        {
          stream_write(stream, data, sizeof data);
          stream_flush(stream);
          write_count --;
        }
      else
        stream_destruct(stream);
      return CORE_OK;
    case STREAM_READ:
      stream_read(stream, &base, &size);
      read_size += size;
      stream_consume(stream, size);
      return CORE_OK;
    case STREAM_CLOSE:
      stream_read(stream, &base, &size);
      read_size += size;
      stream_consume(stream, size);
      stream_destruct(stream);
      return CORE_ABORT;
    case STREAM_ERROR:
      error_count ++;
      stream_destruct(stream);
      return CORE_ABORT;
    }

  return CORE_OK;
}

static void basic_pipe()
{
  char data[1024] = {0};
  int fd[2];
  stream stream;

  core_construct(NULL);

  // pipe close
  pipe(fd);
  fcntl(fd[0], F_SETFL, O_NONBLOCK);
  stream_construct(&stream, callback, &stream);
  stream_open(&stream, fd[0]);
  close(fd[1]);
  assert_true(stream_is_open(&stream));
  core_loop(NULL);
  assert_false(stream_is_open(&stream));

  // pipe message
  pipe(fd);
  fcntl(fd[0], F_SETFL, O_NONBLOCK);
  stream_construct(&stream, callback, &stream);
  stream_open(&stream, fd[0]);
  write(fd[1], data, sizeof data);
  close(fd[1]);
  assert_true(stream_is_open(&stream));
  core_loop(NULL);
  assert_false(stream_is_open(&stream));

  core_destruct(NULL);
}

static void basic_socketpair()
{
  char data[1024] = {0};
  int e, fd[2];
  stream in, out;
  void *base;
  size_t size;

  core_construct(NULL);

  // short message
  e = socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fd);
  assert_true(e == 0);
  fcntl(fd[0], F_SETFL, O_NONBLOCK);
  stream_construct(&in, callback, &in);
  stream_open(&in, fd[0]);
  write(fd[1], data, sizeof data);
  close(fd[1]);
  assert_true(stream_is_open(&in));
  assert_true(stream_is_socket(&in));
  core_loop(NULL);
  assert_false(stream_is_open(&in));

  // long message
  e = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, PF_UNSPEC, fd);
  assert_true(e == 0);
  stream_construct(&in, callback, &in);
  stream_construct(&out, callback, &out);
  stream_open(&in, fd[0]);
  stream_open(&out, fd[1]);
  write_count = 16;
  read_size = 0;
  stream_notify(&out);
  core_loop(NULL);
  assert_int_equal(write_count, 0);
  assert_int_equal(read_size, 16 * 1048576);

  // lines
  e = socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, PF_UNSPEC, fd);
  assert_true(e == 0);
  stream_construct(&in, callback_readline, &in);
  stream_construct(&out, callback_readline, &out);
  stream_open(&in, fd[0]);
  stream_readline(&in, &base, &size);
  assert_int_equal(size, 0);
  stream_open(&out, fd[1]);
  write_count = 1;
  stream_notify(&out);
  core_loop(NULL);
  assert_int_equal(write_count, 0);

  core_destruct(NULL);
}

static void errors()
{
  stream s;

  core_construct(NULL);

  // open invalid
  stream_construct(&s, callback, &s);
  error_count = 0;
  stream_open(&s, -1);
  core_loop(NULL);
  assert_int_equal(error_count, 1);

  // open invalid and destruct
  stream_construct(&s, callback, &s);
  error_count = 0;
  stream_open(&s, -1);
  stream_destruct(&s);
  core_loop(NULL);
  assert_int_equal(error_count, 0);

  // write on closed
  stream_construct(&s, callback, &s);
  stream_write(&s, "" , 0);
  stream_notify(&s);
  stream_flush(&s);
  stream_destruct(&s);

  // double open
  stream_construct(&s, callback, &s);
  stream_open(&s, 0);
  stream_open(&s, 0);
  stream_destruct(&s);

  core_destruct(NULL);
}

int main()
{
  const struct CMUnitTest tests[] =
    {
     cmocka_unit_test(basic_pipe),
     cmocka_unit_test(basic_socketpair),
     cmocka_unit_test(errors)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
