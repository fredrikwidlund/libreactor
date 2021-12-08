#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

typedef struct transfer transfer;
struct transfer
{
  int fd[2];
  stream client;
  size_t client_written;
  size_t client_read;
  stream server;
  size_t server_read;
  char *data;
  size_t size;
};

static void transfer_client_callback(reactor_event *event)
{
  transfer *t = event->state;
  void *data;
  size_t size;

  switch (event->type)
  {
  case STREAM_READ:
    stream_read(&t->client, &data, &size);
    stream_consume(&t->client, size);
    t->client_read += size;
    if (t->client_read == t->size)
    {
      stream_close(&t->client);
      stream_close(&t->server);
    }
    break;
  case STREAM_WRITE:
    assert_int_equal(t->client_written, 0);
    stream_write(&t->client, t->data, t->size);
    stream_flush(&t->client);
    t->client_written = t->size;
    break;
  case STREAM_CLOSE:
  default:
    assert_true(0);
  }
}

static void transfer_server_callback(reactor_event *event)
{
  transfer *t = event->state;
  void *data;
  size_t size;

  switch (event->type)
  {
  case STREAM_READ:
    stream_read(&t->server, &data, &size);
    t->server_read += size;
    stream_write(&t->server, data, size);
    stream_consume(&t->server, size);
    stream_flush(&t->server);
    break;
  case STREAM_WRITE:
  case STREAM_CLOSE:
  default:
    assert_true(0);
    break;
  }
}

static void transfer_run(transfer *t, int ssl, size_t bytes)
{
  SSL_CTX *client_ctx = NULL, *server_ctx = NULL;

  *t = (transfer) {0};
  t->size = bytes;
  t->data = calloc(1, t->size);
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, t->fd), 0);
  if (ssl)
  {
    client_ctx = SSL_CTX_new(TLS_client_method());
    server_ctx = SSL_CTX_new(TLS_server_method());
    assert_int_equal(SSL_CTX_use_certificate_file(server_ctx, "example/certs/cert.pem", SSL_FILETYPE_PEM), 1);
    assert_int_equal(SSL_CTX_use_PrivateKey_file(server_ctx, "example/certs/key.pem", SSL_FILETYPE_PEM), 1);
  }

  reactor_construct();
  stream_construct(&t->client, transfer_client_callback, t);
  stream_open(&t->client, t->fd[0], client_ctx, 1);
  stream_construct(&t->server, transfer_server_callback, t);
  stream_open(&t->server, t->fd[1], server_ctx, 0);

  reactor_loop();
  reactor_destruct();

  stream_destruct(&t->client);
  stream_destruct(&t->server);
  SSL_CTX_free(client_ctx);
  SSL_CTX_free(server_ctx);
  free(t->data);
}

static void transfers(void **arg)
{
  transfer t;

  (void) arg;
  /* transfer */
  transfer_run(&t, 0, 1);
  transfer_run(&t, 0, 256);
  transfer_run(&t, 0, 1048576);

  /* transfer over ssl */
  transfer_run(&t, 1, 1);
  transfer_run(&t, 1, 256);
  transfer_run(&t, 1, 1048576);
}

static void edge(void **arg)
{
  SSL_CTX *client_ctx;
  int fd[2];
  stream client;

  (void) arg;

  /* stream allocation and write notify */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], NULL, 0);
  stream_write_notify(&client);
  stream_allocate(&client, 64);
  stream_destruct(&client);
  close(fd[1]);
  reactor_destruct();

  /* fake SSL_ERROR_WANT_WRITE case  */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  client_ctx = SSL_CTX_new(TLS_client_method());
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], client_ctx, 1);
  client.ssl_state = SSL_ERROR_WANT_WRITE;
  stream_flush(&client);
  stream_destruct(&client);
  SSL_CTX_free(client_ctx);
  reactor_destruct();

  /* provoke ssl error */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fd), 0);
  client_ctx = SSL_CTX_new(TLS_client_method());
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], client_ctx, 1);
  close(fd[1]);
  reactor_loop_once();
  stream_destruct(&client);
  SSL_CTX_free(client_ctx);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(transfers),
          cmocka_unit_test(edge)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
