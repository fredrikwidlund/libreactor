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
  int client_fd;
  stream client;
  int server_fd;
  server server;
  timer timer;
};

static void requests_timer_callback(reactor_event *event)
{
  struct requests *r = event->state;

  stream_write(&r->client, data_construct(r->request, strlen(r->request)));
  stream_flush(&r->client);
  timer_destruct(&r->timer);
}

static void requests_client_callback(reactor_event *event)
{
  const char expect200[] = "HTTP/1.1 200 OK\r\n";
  const char expect404[] = "HTTP/1.1 404 Not Found\r\n";
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
      stream_flush(&r->client);
      timer_construct(&r->timer, requests_timer_callback, r);
      timer_set(&r->timer, 1000000, 0);
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
      assert_true(strlen(expect200) < data_size(data) && strncmp(data_base(data), expect200, strlen(expect200)) == 0);
      break;
    case 404:
      assert_true(strlen(expect404) < data_size(data) && strncmp(data_base(data), expect404, strlen(expect404)) == 0);
      break;
    }
    stream_close(&r->client);
    server_shutdown(&r->server);
    break;
  case STREAM_CLOSE:
    assert_int_equal(r->code, 0);
    stream_close(&r->client);
    server_shutdown(&r->server);
    break;
  }
}

struct task
{
  server_request *request;
  timer           timer;
};

static void timeout(reactor_event *event)
{
  struct task *task = event->state;

  server_ok(task->request, data_string("text/plain"), data_string("Hello from the future, World!"));
  timer_destruct(&task->timer);
  free(task);
}

static void requests_server_callback(reactor_event *event)
{
  server_request *request = (server_request *) event->data;
  struct task *task;

  if (data_equal(request->target, data_string("/")))
    server_ok(request, data_string("plain/text"), data_string("hello"));
  else if (data_equal(request->target, data_string("/wait")))
  {
    task = malloc(sizeof *task);
    task->request = request;
    timer_construct(&task->timer, timeout, task);
    timer_set(&task->timer, 1000000000, 0);
  }
  else if (data_equal(request->target, data_string("/close")))
  {
    server_close(request);
    /* below should be ignored now */
    server_close(request);
    server_hold(request);
    server_ok(request, data_string("plain/text"), data_string("hello"));
    server_hold(request);
    server_bad_request(request);
  }
  else if (data_equal(request->target, data_string("/bad")))
  {
    server_bad_request(request);
  }
  else
    server_not_found(request);
}

static void requests_run(char *request, int code, int partial, int ssl)
{
  struct requests r = {.request = request, .code = code, .partial = partial};
  SSL_CTX *client_ssl_ctx = NULL, *server_ssl_ctx = NULL;

  if (ssl)
  {
    client_ssl_ctx = SSL_CTX_new(TLS_client_method());
    assert_true(client_ssl_ctx);
    server_ssl_ctx = net_ssl_server_context("test/files/cert.pem", "test/files/key.pem");
    assert_true(server_ssl_ctx);
  }

  r.server_fd = net_socket(net_resolve("127.0.0.1", "12345", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_true(r.server_fd >= 0);
  r.client_fd = net_socket(net_resolve("127.0.0.1", "12345", AF_INET, SOCK_STREAM, 0));
  assert_true(r.client_fd >= 0);

  reactor_construct();
  stream_construct(&r.client, requests_client_callback, &r);
  stream_open(&r.client, r.client_fd, STREAM_READ | STREAM_WRITE, client_ssl_ctx);
  stream_notify(&r.client);
  server_construct(&r.server, requests_server_callback, &r);
  server_open(&r.server, r.server_fd, server_ssl_ctx);
  reactor_loop();
  stream_destruct(&r.client);
  server_destruct(&r.server);
  reactor_destruct();

  SSL_CTX_free(client_ssl_ctx);
  SSL_CTX_free(server_ssl_ctx);
}

static void test_env(void **arg)
{
  (void) arg;
  assert_int_equal(chdir(DATADIR), 0);
}

static void test_requests(void **arg)
{
  (void) arg;
  requests_run("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0, 0);
  requests_run("GET /wait HTTP/1.1\r\nHost: localhost\r\n\r\nxxxxxxxxxxxxxxx", 200, 39, 0);
  requests_run("GET /wait HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0, 0);
  requests_run("GET /doesntexit HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, 0, 0);
  requests_run("invalid\r\n\r\n", 400, 0, 0);
  requests_run("GET /close HTTP/1.1\r\nHost: localhost\r\n\r\n", 0, 0, 0);
  requests_run("GET /bad HTTP/1.1\r\nHost: localhost\r\n\r\n", 400, 0, 0);
}

static void test_requests_ssl(void **arg)
{
  (void) arg;
  requests_run("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0, 1);
  requests_run("GET /wait HTTP/1.1\r\nHost: localhost\r\n\r\nxxxxxxxxxxxxxxx", 200, 39, 1);
  requests_run("GET /wait HTTP/1.1\r\nHost: localhost\r\n\r\n", 200, 0, 1);
  requests_run("GET /doesntexit HTTP/1.1\r\nHost: localhost\r\n\r\n", 404, 0, 1);
  requests_run("invalid\r\n\r\n", 400, 0, 1);
  requests_run("GET /close HTTP/1.1\r\nHost: localhost\r\n\r\n", 0, 0, 1);
}

static void test_edge_cases(void **arg)
{
  int fd[2];
  stream client;
  server server;
  SSL_CTX *server_ssl_ctx;

  (void) arg;
  /* disconnecting client */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], STREAM_WRITE, NULL);
  server_construct(&server, NULL, NULL);
  server_accept(&server, fd[1]);
  stream_close(&client);
  reactor_loop_once();
  stream_destruct(&client);
  server_destruct(&server);
  reactor_destruct();

  /* accept tls */
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, fd), 0);
  reactor_construct();
  stream_construct(&client, NULL, NULL);
  stream_open(&client, fd[0], STREAM_WRITE, NULL);
  server_construct(&server, NULL, NULL);
  server_ssl_ctx = net_ssl_server_context("test/files/cert.pem", "test/files/key.pem");
  assert_true(server_ssl_ctx);
  server.ssl_ctx = server_ssl_ctx;
  server_accept(&server, fd[1]);
  stream_close(&client);
  stream_destruct(&client);
  server_destruct(&server);
  reactor_destruct();
  SSL_CTX_free(server_ssl_ctx);

  /* close already closed server */
  fd[0] = dup(0);
  reactor_construct();
  server_construct(&server, NULL, NULL);
  server_open(&server, fd[0], NULL);
  server_shutdown(&server);
  server_shutdown(&server);
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
  stream_open(&client, fd[0], STREAM_WRITE, NULL);
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
          cmocka_unit_test(test_env),
          cmocka_unit_test(test_requests),
          cmocka_unit_test(test_requests_ssl),
          cmocka_unit_test(test_edge_cases)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
