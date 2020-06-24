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
#include <sys/types.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

static int write_count = 0;
static int read_size =  0;
static int error_count = 0;

static core_status callback_readline(core_event *event)
{
  stream *stream = event->state;
  segment s;

  switch (event->type)
    {
    case STREAM_FLUSH:
      if (write_count)
        {
          s = stream_allocate(stream, 5);
          memcpy(s.base, "line\n", 5);
          stream_flush(stream);
          write_count --;
        }
      else
        stream_destruct(stream);
      return CORE_OK;
    case STREAM_READ:
      s = stream_read_line(stream);
      assert_true(segment_equal(s, segment_string("line\n")));
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
  segment s;

  switch(event->type)
    {
    case STREAM_FLUSH:
      if (write_count)
        {
          stream_write(stream, segment_data(data, sizeof data));
          stream_flush(stream);
          write_count --;
        }
      else
        stream_destruct(stream);
      return CORE_OK;
    case STREAM_READ:
      s = stream_read(stream);
      read_size += s.size;
      stream_consume(stream, s.size);
      return CORE_OK;
    case STREAM_CLOSE:
      stream_destruct(stream);
      return CORE_ABORT;
    case STREAM_ERROR:
      error_count ++;
      stream_destruct(stream);
      return CORE_ABORT;
    }

  return CORE_OK;
}

static void basic_pipe(__attribute__ ((unused)) void **state)
{
  char data[1024] = {0};
  int fd[2];
  stream stream, out;

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
  stream_construct(&out, callback, &out);
  stream_open(&out, fd[1]);
  stream_write(&out, segment_data(data, sizeof data));
  stream_flush(&out);
  stream_destruct(&out);
  assert_true(stream_is_open(&stream));
  core_loop(NULL);
  assert_false(stream_is_open(&stream));

  core_destruct(NULL);
}

static void basic_socketpair(__attribute__ ((unused)) void **state)
{
  char data[1024] = {0};
  int e, fd[2];
  stream in, out;
  segment s;

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
  s = stream_read_line(&in);
  assert_int_equal(s.size, 0);
  stream_open(&out, fd[1]);
  write_count = 1;
  stream_notify(&out);
  core_loop(NULL);
  assert_int_equal(write_count, 0);

  core_destruct(NULL);
}

static void errors(__attribute__ ((unused)) void **state)
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
  stream_write(&s, segment_empty());
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
