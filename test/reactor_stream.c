#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

extern int reactor_stream_event(void *, int, void *);
extern int mock_read_failure;
extern int mock_write_failure;

struct transport
{
  reactor_stream stream;
  size_t         read;
};

int transport_event(void *state, int type, void *data)
{
  struct transport *t = state;
  struct iovec *iov;

  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      iov = data;
      assert_true(iov->iov_len <= t->read);
      t->read -= iov->iov_len;
      *iov = (struct iovec) {0};
      if (!t->read && !reactor_stream_blocked(state))
        {
          reactor_stream_close(&t->stream);
          return REACTOR_ABORT;
        }
      return REACTOR_OK;
    case REACTOR_STREAM_EVENT_WRITE:
      assert_int_equal(reactor_stream_flush(state), REACTOR_OK);
      if (!t->read && !reactor_stream_blocked(state))
        {
          reactor_stream_close(&t->stream);
          return REACTOR_ABORT;
        }
      return REACTOR_OK;
    case REACTOR_STREAM_EVENT_CLOSE:
      reactor_stream_close(state);
      return REACTOR_ABORT;
    default:
      assert_true(0);
      return REACTOR_OK;
    }
}

void transport_data(int fd[2], size_t size)
{
  char *data;
  struct transport t1, t2;

  data = calloc(1, size);
  assert_true(data);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  t1.read = size;
  t2.read = size / 2;
  assert_int_equal(reactor_stream_open(&t1.stream, transport_event, &t1, fd[0]), REACTOR_OK);
  assert_true(reactor_stream_buffer(&t1.stream) != NULL);
  assert_int_equal(reactor_stream_open(&t2.stream, transport_event, &t2, fd[1]), REACTOR_OK);
  reactor_stream_write(&t1.stream, data, size / 2);
  reactor_stream_write(&t2.stream, data, size);
  assert_int_equal(reactor_stream_flush(&t1.stream), REACTOR_OK);
  assert_int_equal(reactor_stream_flush(&t2.stream), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();
  free(data);
}

void transport_short(int fd[2])
{
  transport_data(fd, 16);
}

void transport_long(int fd[2])
{
  transport_data(fd, 16 * 1024 * 1024);
}

static void socket_pipe(int fd[2])
{
  assert_int_equal(pipe(fd), 0);
}

static void socket_socketpair(int fd[2])
{
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, AF_UNSPEC, fd), 0);
}

static void socket_tcp(int fd[2])
{
  struct sockaddr_in sin = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK), .sin_port = htons(12346)};
  int server;

  server = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  assert_true(server >= 0);
  (void) setsockopt(server, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  assert_int_equal(bind(server, (struct sockaddr *) &sin, sizeof sin), 0);
  assert_int_equal(listen(server, -1), 0);

  fd[1] = socket(AF_INET, SOCK_STREAM, 0);
  assert_true(fd[1] >= 0);
  assert_int_equal(connect(fd[1], (struct sockaddr *) &sin, sizeof sin), 0);

  fd[0] = accept(server, NULL, NULL);
  assert_true(fd[0] >= 0);
  assert_int_equal(close(server), 0);
}

void core()
{
  int fd[2];

  socket_socketpair(fd);
  transport_short(fd);
  socket_socketpair(fd);
  transport_long(fd);
  socket_tcp(fd);
  transport_short(fd);
  socket_tcp(fd);
  transport_long(fd);
}

struct error_state
{
  int            fd[2];
  reactor_stream stream;
};

int error_event1(void *state, int type, void *data)
{
  struct error_state *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      mock_read_failure = EIO;
      assert_int_equal(write(s->fd[1], "", 1), 1);
      return REACTOR_OK;
    default:
      reactor_stream_close(&s->stream);
      return REACTOR_ABORT;
    }
}

int error_event2(void *state, int type, void *data)
{
  struct error_state *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      close(s->fd[1]);
      return REACTOR_OK;
    default:
      reactor_stream_close(&s->stream);
      return REACTOR_ABORT;
    }
}

void error()
{
  int fd;
  char buffer[REACTOR_STREAM_BLOCK_SIZE] = {0};
  struct error_state s;
  ssize_t n;

  // open file
  fd = open("/dev/null", O_RDONLY);
  assert_true(fd != -1);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event1, &s, fd), REACTOR_ERROR);
  reactor_core_destruct();
  close(fd);

  // read error when not buffered
  socket_pipe(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event1, &s, s.fd[0]), REACTOR_OK);
  assert_int_equal(write(s.fd[1], "", 1), 1);
  mock_read_failure = EIO;
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();
  close(s.fd[1]);
  mock_read_failure = 0;

  // read error when buffered
  socket_pipe(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event1, &s, s.fd[0]), REACTOR_OK);
  assert_int_equal(write(s.fd[1], "", 1), 1);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();
  close(s.fd[1]);
  mock_read_failure = 0;

  // write error
  socket_socketpair(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event1, &s, s.fd[0]), REACTOR_OK);
  while (!reactor_stream_blocked(&s.stream))
    {
      reactor_stream_write(&s.stream, buffer, sizeof buffer);
      assert_int_equal(reactor_stream_flush(&s.stream), REACTOR_OK);
    }
  while (1)
    {
      n = read(s.fd[1], buffer, sizeof buffer);
      if (n <= 0)
        break;
    }
  assert_true(reactor_stream_blocked(&s.stream));
  mock_write_failure = EIO;
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_stream_close(&s.stream);
  reactor_core_destruct();
  close(s.fd[1]);
  mock_write_failure = 0;

  // write error when blocked
  socket_pipe(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event1, &s, s.fd[0]), REACTOR_OK);
  reactor_stream_write_string(&s.stream, "test");
  mock_write_failure = EIO;
  assert_int_equal(reactor_stream_flush(&s.stream), REACTOR_ERROR);
  reactor_stream_close(&s.stream);
  reactor_core_destruct();
  close(s.fd[1]);
  mock_write_failure = 0;

  // read close
  socket_pipe(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event2, &s, s.fd[0]), REACTOR_OK);
  assert_int_equal(write(s.fd[1], "", 1), 1);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();
  close(s.fd[1]);

  // invalid event
  socket_pipe(s.fd);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_stream_open(&s.stream, error_event2, &s, s.fd[0]), REACTOR_OK);
  assert_int_equal(reactor_stream_event(&s.stream, 0, (int[]){0}), REACTOR_ABORT);
  reactor_core_destruct();
  close(s.fd[1]);
}

int exception_event1(void *state, int type, void *data)
{
  reactor_stream *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_STREAM_EVENT_READ:
      return REACTOR_OK;
    default:
      reactor_stream_close(s);
      return REACTOR_ABORT;
    }
}


int exception_event2(void *state, int type, void *data)
{
  reactor_stream *s = state;

  (void) type;
  (void) data;
  reactor_stream_close(s);
  return REACTOR_ABORT;
}

void exception()
{
  reactor_stream s;
  int fd[2];
  char buffer[REACTOR_STREAM_BLOCK_SIZE] = {0};
  ssize_t n;

  assert_int_equal(reactor_core_construct(), REACTOR_OK);

  // read and close
  socket_socketpair(fd);
  write(fd[1], "test", 4);
  close(fd[1]);
  assert_int_equal(reactor_stream_open(&s, exception_event1, &s, fd[0]), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // read and read that returns 0
  socket_socketpair(fd);
  assert_int_equal(setsockopt(fd[1], SOL_SOCKET, SO_SNDBUF, (int[]){sizeof buffer}, sizeof(int)), 0);
  write(fd[1], buffer, sizeof buffer);
  close(fd[1]);
  assert_int_equal(reactor_stream_open(&s, exception_event1, &s, fd[0]), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on read when output is waiting
  socket_socketpair(fd);
  assert_int_equal(reactor_stream_open(&s, exception_event2, &s, fd[0]), REACTOR_OK);
  while (!reactor_stream_blocked(&s))
    {
      reactor_stream_write(&s, buffer, sizeof buffer);
      assert_int_equal(reactor_stream_flush(&s), REACTOR_OK);
    }
  while (1)
    {
      n = read(fd[1], buffer, sizeof buffer);
      if (n <= 0)
        break;
    }
  write(fd[1], "", 1);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  close(fd[1]);

  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core),
    cmocka_unit_test(error),
    cmocka_unit_test(exception)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
