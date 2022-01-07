#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

struct transfer_state
{
  stream from;
  stream to;
  size_t remaining;
  int    notified;
};

static void transfer_writer(reactor_event *event)
{
  struct transfer_state *state = event->state;

  assert_int_equal(event->type, STREAM_WRITE);
  state->notified = 1;
  if (!state->remaining)
  {
    stream_destruct(&state->from);
    stream_destruct(&state->to);
  }
}

static void transfer_reader(reactor_event *event)
{
  struct transfer_state *state = event->state;
  data data;

  assert_int_equal(event->type, STREAM_READ);
  data = stream_read(&state->to);
  stream_consume(&state->to, data_size(data));
  state->remaining -= data_size(data);
  if (!state->remaining && state->notified)
  { 
    stream_destruct(&state->from);
    stream_destruct(&state->to);
  }
}

static int transfer(int from, int to, int ssl, size_t size)
{
  SSL_CTX *client_ctx = NULL, *server_ctx = NULL;
  struct transfer_state state = {.remaining = size};
  char data[size];

  if (ssl)
  {
    client_ctx = SSL_CTX_new(TLS_client_method());
    server_ctx = net_ssl_server_context("test/files/cert.pem", "test/files/key.pem");
    assert(client_ctx && server_ctx);
  }
 
  memset(data, 0, size);
  from = dup(from);
  to = dup(to);
  assert_int_equal(fcntl(from, F_SETFL, O_NONBLOCK), 0);
  assert_int_equal(fcntl(to, F_SETFL, O_NONBLOCK), 0);
  
  reactor_construct();
  stream_construct(&state.from, transfer_writer, &state);
  stream_open(&state.from, from, STREAM_WRITE, client_ctx);
  stream_construct(&state.to, transfer_reader, &state);
  stream_open(&state.to, to, STREAM_READ, server_ctx);
  stream_write(&state.from, data_construct(data, size));
  stream_flush(&state.from);
  stream_notify(&state.from);
  reactor_loop();
  reactor_destruct();

  assert_int_equal(state.notified, 1);
  SSL_CTX_free(client_ctx);
  SSL_CTX_free(server_ctx);
  return 0;
}

static void test_stream_transfer(__attribute__((unused)) void **arg)
{
  int fd[2];
  
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 1), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 1024), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 65536), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 2000000), 0);
  assert_int_equal(transfer(fd[1], fd[0], 1, 2000000), 0);
  close(fd[0]);
  close(fd[1]);

  assert_int_equal(pipe(fd), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 1), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 1024), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 65536), 0);
  assert_int_equal(transfer(fd[1], fd[0], 0, 2000000), 0);
  close(fd[0]);
  close(fd[1]);
}

static void test_stream(__attribute__((unused)) void **arg)
{
  int fd[2];
  stream reader;
  SSL_CTX *ssl_ctx;

  /* socket close */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  reactor_construct();
  stream_construct(&reader, NULL, NULL);
  stream_open(&reader, fd[1], STREAM_READ, NULL);
  close(fd[0]);
  reactor_loop_once();
  stream_destruct(&reader);
  reactor_destruct();

  /* socket ssl close */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  ssl_ctx = SSL_CTX_new(TLS_client_method());
  reactor_construct();
  stream_construct(&reader, NULL, NULL);
  stream_open(&reader, fd[1], STREAM_READ, ssl_ctx);
  close(fd[0]);
  reactor_loop_once();
  stream_destruct(&reader);
  reactor_destruct();
  SSL_CTX_free(ssl_ctx);

  /* notify/close on inactive */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  reactor_construct();
  stream_construct(&reader, NULL, NULL);
  stream_open(&reader, fd[1], STREAM_READ, NULL);
  close(fd[0]);
  stream_close(&reader);
  stream_notify(&reader);
  stream_close(&reader);
  stream_destruct(&reader);
  reactor_destruct();

  /* allocate */
  reactor_construct();
  stream_construct(&reader, NULL, NULL);
  assert_true(stream_allocate(&reader, 4096) != NULL);
  stream_destruct(&reader);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
      {
        cmocka_unit_test(test_stream_transfer),
        cmocka_unit_test(test_stream)
      };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
