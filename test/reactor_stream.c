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
extern int debug_out_of_memory;

static int reader_called = 0;
static int reader_error = 0;

void reader_event(void *state, int type, void *data)
{
  reactor_stream *reader = state;
  reactor_stream_data *in;

  assert_true(reader);
  assert_true(type);
  assert_true(data);

  if (type & REACTOR_STREAM_DATA)
    {
      in = data;
      if (in->size == 1048576)
        {
          reactor_stream_data_consume(in, in->size);
          reactor_stream_close(reader);
        }
    }

  if (type & REACTOR_STREAM_END)
    reactor_stream_close(reader);

  if (type & REACTOR_STREAM_ERROR)
    reader_error ++;

  reader_called ++;
}

static int writer_called = 0;

void writer_event(void *state, int type, void *data)
{
  reactor_stream *writer = state;

  assert_true(writer);
  assert_true(type);
  assert_true(data);

  writer_called ++;
}

void coverage()
{
  int e;

  reactor_core_construct();

  /* run reactor */
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(reader_called == 0);

  reactor_core_destruct();
}

void basic()
{
  reactor_stream reader, writer;
  char data[1048576];
  int e, fds[2];
  ssize_t n;

  /* construct reactor */
  e = reactor_core_construct();
  assert_int_equal(e, 0);

  /* open invalid fd */
  reactor_stream_init(&reader, reader_event, &reader);
  e = reactor_stream_open(&reader, -1);
  assert_int_equal(e, -1);

  /* small write */
  e = pipe(fds);
  assert_int_equal(e, 0);
  e = reactor_stream_open(&reader, fds[0]);
  assert_int_equal(e, 0);
  reactor_stream_init(&writer, writer_event, &writer);
  e = reactor_stream_open(&writer, fds[1]);
  assert_int_equal(e, 0);
  e = reactor_stream_printf(&writer, "test");
  assert_int_equal(e, 0);
  reactor_stream_close(&writer);
  reactor_stream_flush(&writer);

  /* write on closed fd */
  close(fds[1]);
  e = reactor_stream_printf(&writer, "test");
  assert_int_equal(e, 0);
  reactor_stream_flush(&writer);

  /* run */
  e = reactor_core_run();
  assert_int_equal(e, 0);

  /* large write */
  memset(data, '.', sizeof data);
  reactor_stream_init(&writer, writer_event, &writer);
  reactor_stream_init(&reader, reader_event, &reader);
  e = pipe(fds);
  assert_int_equal(e, 0);
  e = reactor_stream_open(&reader, fds[0]);
  assert_int_equal(e, 0);
  e = reactor_stream_open(&writer, fds[1]);
  assert_int_equal(e, 0);
  e = reactor_stream_write(&writer, data, sizeof data);
  assert_int_equal(e, 0);
  reactor_stream_close(&writer);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  /* simulate read error */
  e = pipe(fds);
  assert_int_equal(e, 0);
  e = reactor_stream_open(&reader, fds[0]);
  assert_int_equal(e, 0);
  n = write(fds[1], ".", 1);
  assert_int_equal(n, 1);
  close(fds[1]);
  debug_io_error = 1;
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(reader_error > 0);
  debug_io_error = 0;

  /* open reader stream */
  e = pipe(fds);
  assert_int_equal(e, 0);
  e = reactor_stream_open(&reader, fds[0]);
  assert_int_equal(e, 0);

  /* open already open stream */
  e = reactor_stream_open(&reader, fds[0]);
  assert_int_equal(e, -1);

  /* open writer stream */
  reactor_stream_init(&writer, writer_event, &writer);
  e = reactor_stream_open(&writer, fds[1]);
  assert_int_equal(e, 0);

  /* invalid printf */
  e = reactor_stream_printf(&writer, "%z");
  assert_int_equal(e, -1);

  /* printf alloc fail */
  debug_out_of_memory = 1;
  e = reactor_stream_printf(&writer, data, sizeof data);
  assert_int_equal(e, -1);
  debug_out_of_memory = 0;

  /* write */
  e = reactor_stream_write(&writer, data, sizeof data / 2);
  reactor_stream_flush(&writer);
  assert_int_equal(e, 0);

  /* printf */
  e = reactor_stream_printf(&writer, "%.*s", sizeof data / 2, data + (sizeof data / 2));
  assert_int_equal(e, 0);

  /* run reactor */
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_true(reader_called);
  assert_true(writer_called);

  /* destruct reactor */
  reactor_core_destruct();
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(coverage),
    cmocka_unit_test(basic)
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
