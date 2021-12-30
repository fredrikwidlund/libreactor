#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

struct requests
{
  char *request;
  int code;
  int partial;
  int fd[2];
  stream client;
  server server;
};

static void requests_client_callback(reactor_event *event)
{
  struct requests *r = event->state;
  data data;

  switch (event->type)
  {
  case STREAM_WRITE:
    if (r->partial)
    {
      assert_true((size_t) r->partial < strlen(r->request));
      stream_write(&r->client, data_construct(r->request, r->partial));
      r->request += r->partial;
      r->partial = 0;
      stream_flush(&r->client);
      stream_write_notify(&r->client);
    }
    else
    {
      stream_write(&r->client, data_construct(r->request, strlen(r->request)));
      stream_flush(&r->client);
    }
    break;
  case STREAM_READ:
    data = stream_read(&r->client);
    switch (r->code)
    {
    case 200:
      const char expect200[] = "HTTP/1.1 200 OK\r\n";
      assert_true(strlen(expect200) < data_size(data) && strncmp(data_base(data), expect200, strlen(expect200)) == 0);
      break;
    case 404:
      const char expect404[] = "HTTP/1.1 404 Not Found\r\n";
      assert_true(strlen(expect404) < data_size(data) && strncmp(data_base(data), expect404, strlen(expect404)) == 0);
      break;
    }
    stream_close(&r->client);
    server_close(&r->server);
    break;
  case STREAM_CLOSE:
    assert_int_equal(r->code, 0);
    stream_close(&r->client);
    server_close(&r->server);
    break;
  }
}

static void requests_server_callback(reactor_event *event)
{
  server_transaction *t = (server_transaction *) event->data;
  http_request *r = t->request;

  if (data_equal(r->target, data_string("/manual")))
  {
    server_transaction_write(t, data_string("HTTP/1.1 200 OK\r\n\r\n"));
    server_transaction_ready(t);
  }
  else if (data_equal(r->target, data_string("/text")))
    server_transaction_text(t, data_string("hi there\n"));
  else if (data_equal(r->target, data_string("/")))
    server_transaction_ok(t, data_string("plain/text"), data_string("ok"));
  else if (data_equal(r->target, data_string("/printf")))
    server_transaction_printf(t, data_string("plain/text"), "%d", 42);
  else if (data_equal(r->target, data_string("/close")))
    server_transaction_disconnect(t);
  else
    server_transaction_not_found(t);
}

static void requests_run(char *request, int code, int partial)
{
  struct requests r = {.request = request, .code = code, .partial = partial};

  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, r.fd), 0);
  reactor_construct();
  stream_construct(&r.client, requests_client_callback, &r);
  stream_open(&r.client, r.fd[0], NULL, 1);
  server_construct(&r.server, requests_server_callback, &r);
  server_accept(&r.server, r.fd[1], NULL);
  usleep(100000);
  reactor_loop();
  stream_destruct(&r.client);
  server_destruct(&r.server);
  reactor_destruct();
}

static void test_requests(void **arg)
{
  (void) arg;
  requests_run("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0);
  requests_run("GET /manual HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0);
  requests_run("GET /doesntexit HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, 0);
  requests_run("invalid\r\n\r\n", 0, 0);
  requests_run("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 4);
  requests_run("GET /text HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0);
  requests_run("GET /printf HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0);
  requests_run("GET /close HTTP/1.1\r\nHost: localhost\r\n\r\n", 0, 0);
}

static void test_edge_cases(void **arg)
{
  int fd[2];
  stream client;
  server server;

  (void) arg;

  /* disconnecting client */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], NULL, 1);
  server_construct(&server, NULL, NULL);
  server_accept(&server, fd[1], NULL);
  stream_close(&client);
  reactor_loop_once();
  stream_destruct(&client);
  server_destruct(&server);
  reactor_destruct();

  /* server accept and shutdown */
  struct sockaddr_in sin;
  socklen_t sin_len = sizeof sin;
  char port[16];

  fd[1] = net_socket(net_resolve("127.0.0.1", "0", AF_INET, SOCK_STREAM, AI_PASSIVE));
  getsockname(fd[1], (struct sockaddr *) &sin, &sin_len);
  snprintf(port, sizeof port, "%d", ntohs(sin.sin_port));
  fd[0] = net_socket(net_resolve("127.0.0.1", port, AF_INET, SOCK_STREAM, 0));
  assert_true(fd[0] > 0 && fd[1] > 0);

  reactor_construct();
  server_construct(&server, NULL, NULL);
  server_open(&server, fd[1], NULL);
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], NULL, 1);
  reactor_loop_once();
  server_shutdown(&server);
  reactor_loop_once();
  stream_destruct(&client);
  server_destruct(&server);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_requests),
          cmocka_unit_test(test_edge_cases)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
