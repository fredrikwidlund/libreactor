#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <sys/param.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int mock_read_failure;

static int reader_data = 0;
static int reader_read = 0;
static int reader_write = 0;
static int reader_hangup = 0;
static int reader_error = 0;
static int reader_close = 0;

void core_reader(void *state, int type, void *event_data)
{
  reactor_stream *stream = state;
  reactor_stream_data *data = event_data;

  switch(type)
    {
    case REACTOR_STREAM_EVENT_READ:
      reader_data += data->size;
      data->size = 0;
      reader_read ++;
      break;
    case REACTOR_STREAM_EVENT_WRITE:
      reader_write ++;
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      reader_hangup ++;
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_EVENT_ERROR:
      reader_error ++;
      reactor_stream_close(stream);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      reader_close ++;
      break;
    }
}

void core_writer(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_ERROR:
      break;
    case REACTOR_STREAM_EVENT_WRITE:
      reactor_stream_close(stream);
      break;
    }
}

void core()
{
  reactor_stream reader, writer;
  int e, fds[2], fd;

  reactor_core_construct();

  e = pipe(fds);
  assert_int_equal(e, 0);
  reactor_stream_open(&reader, core_reader, &reader, fds[0]);
  reactor_stream_open(&writer, core_writer, &writer, fds[1]);
  reactor_stream_hold(&writer);
  reactor_stream_write(&writer, "test1", 5);
  reactor_stream_write(&writer, "test2", 5);
  reactor_stream_write(&writer, "test3", 5);
  reactor_stream_close(&writer);
  reactor_stream_release(&writer);

  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(reader_data, 15);

  reader_error = 0;
  reactor_stream_open(&reader, core_reader, &reader, 1000);
  e = reactor_core_run();
  assert_int_equal(e, 0);
  assert_int_equal(reader_error, 1);

  reactor_stream_open(&writer, core_writer, &writer, 1000);
  reactor_stream_write(&writer, "test", 4);
  reactor_stream_flush(&writer);
  reactor_stream_close(&writer);

  reactor_stream_open(&writer, core_writer, &writer, 1000);
  reactor_stream_write(&writer, "test", 4);
  reactor_stream_close(&writer);

  fd = dup(1);
  reactor_stream_open(&writer, core_writer, &writer, fd);
  reactor_stream_write_notify(&writer);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

struct pump_object
{
  int blocked;
  reactor_stream writer;
  reactor_stream reader;
  size_t write;
  size_t read;
};

void pump_writer(void *state, int type, void *data)
{
  struct pump_object *po = state;
  char buffer[65536] = {0};
  size_t n;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_WRITE:
      po->blocked = 0;
      while (!po->blocked && po->write)
        {
          n = MIN(po->write, sizeof (buffer));
          reactor_stream_write(&po->writer, buffer, n);
          po->write -= n;
          reactor_stream_flush(&po->writer);
        }
      if (!po->write)
        reactor_stream_close(&po->writer);
      break;
    case REACTOR_STREAM_EVENT_BLOCKED:
      po->blocked = 1;
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      break;
    }
}

void pump_reader(void *state, int type, void *data)
{
  struct pump_object *po = state;
  reactor_stream_data *read = data;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      po->read -= read->size;
      reactor_stream_data_consume(read, read->size);
      break;
    case REACTOR_STREAM_EVENT_HANGUP:
      assert_true(po->read == 0);
      reactor_stream_close(&po->reader);
      break;
    case REACTOR_STREAM_EVENT_CLOSE:
      break;
    }
}

void pump()
{
  struct pump_object po;
  int e, fds[2];

  reactor_core_construct();

  e = pipe(fds);
  assert_int_equal(e, 0);
  po.write = 104857600;
  po.read = 104857600;
  po.blocked = 1;
  reactor_stream_open(&po.reader, pump_reader, &po, fds[0]);
  reactor_stream_open(&po.writer, pump_writer, &po, fds[1]);
  reactor_stream_write_notify(&po.writer);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

void file_reader(void *state, int type, void *data)
{
  reactor_stream *stream = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_ERROR:
    case REACTOR_STREAM_EVENT_HANGUP:
      reactor_stream_close(stream);
      break;
    }
}

void file()
{
  int fd, e;
  reactor_stream reader;

  reactor_core_construct();

  /* file eof */
  fd = open("Makefile", O_RDONLY);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_reader, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  /* read error */
  fd = open("Makefile", O_RDONLY);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_reader, &reader, fd);
  mock_read_failure = EIO;
  e = reactor_core_run();
  assert_int_equal(e, 0);

  /* read would block */
  fd = open("Makefile", O_RDONLY);
  assert_true(fd != -1);
  mock_read_failure = EAGAIN;
  reactor_stream_open(&reader, file_reader, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  /* close on already closed stream */
  reactor_stream_close(&reader);

  reactor_core_destruct();
}

void sock()
{
  int fd, e;
  reactor_stream reader;

  reactor_core_construct();

  fd = socket(AF_INET, SOCK_STREAM, PF_UNSPEC);
  assert_true(fd != -1);
  reactor_stream_open(&reader, file_reader, &reader, fd);
  e = reactor_core_run();
  assert_int_equal(e, 0);

  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(pump),
    cmocka_unit_test(file),
    cmocka_unit_test(sock)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
