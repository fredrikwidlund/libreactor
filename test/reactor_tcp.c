#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

struct state
{
  int         type;
  reactor_tcp server;
  reactor_tcp client;
};

extern int mock_accept_failure;
extern int mock_bind_failure;
extern int mock_listen_failure;
extern int mock_connect, mock_connect_return, mock_connect_errno;
extern int mock_socket_failure;

static int listen_port(void)
{
  struct sockaddr_in sin = {0};
  socklen_t len;
  int fd, e;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  assert_true(fd >= 0);
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  e = bind(fd, (struct sockaddr *) &sin, sizeof sin);
  assert_int_equal(e, 0);
  len = sizeof sin;
  e = getsockname(fd, (struct sockaddr *) &sin, &len);
  assert_int_equal(e, 0);
  (void) close(fd);

  return ntohs(sin.sin_port);
}

static int server_event(void *state, int type, void *data)
{
  struct state *s = state;

  (void) type;
  switch (s->type)
    {
    case 0:
      if (data)
        (void) close(*(int *) data);
      reactor_tcp_close(&s->server);
      return REACTOR_ABORT;
    case 1:
      if (data)
        (void) close(*(int *) data);
      return REACTOR_OK;
    }

  return REACTOR_OK;
}

static int client_event(void *state, int type, void *data)
{
  struct state *s = state;

  (void) type;
  (void) data;
  switch (s->type)
    {
    case 0:
      reactor_tcp_close(&s->client);
      return REACTOR_ABORT;
    case 1:
      reactor_tcp_close(&s->server);
      reactor_tcp_close(&s->client);
      return REACTOR_ABORT;
    }

  return REACTOR_OK;
}

int resolver_event(void *state, int type, void *data)
{
  (void) state;
  (void) type;
  (void) data;
  return REACTOR_OK;
}

static void core()
{
  reactor_resolver r;
  struct state state = {0};
  char port[16];
  int e;

  snprintf(port, sizeof port, "%d", listen_port());
  reactor_core_construct();

  reactor_resolver_open(&r, resolver_event, &r, "127.0.0.1", port, 0, 0, AI_PASSIVE);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // failed connect
  state.type = 0;
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // successful connect
  state.type = 0;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // server socket error
  state.type = 0;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_OK);
  shutdown(state.server.descriptor.fd, SHUT_RD);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // accept error
  state.type = 0;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  mock_accept_failure = ENOMEM;
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // bind error
  state.type = 1;
  mock_bind_failure = EIO;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_ERROR);

  // invalid address
  state.type = 0;
  e = reactor_tcp_open(&state.client, client_event, &state, "", "0", REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // socket failure
  mock_socket_failure = ENOMEM;
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_ERROR);

  // connect failure
  mock_connect = 1;
  mock_connect_errno = EIO;
  mock_connect_return = -1;
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_ERROR);

  // listen error
  mock_listen_failure = EINVAL;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_ERROR);

  // normal connect, client closes both
  state.type = 1;
  e = reactor_tcp_open_addrinfo(&state.server, server_event, &state, r.addrinfo, REACTOR_TCP_FLAG_SERVER);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // successful resolve
  state.type = 1;
  e = reactor_tcp_open(&state.client, client_event, &state, "localhost", port, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // successful resolve but unable to configure
  state.type = 1;
  mock_connect = 1;
  mock_connect_errno = EIO;
  mock_connect_return = -1;
  e = reactor_tcp_open(&state.client, client_event, &state, "localhost", port, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  e = reactor_core_run();
  assert_int_equal(e, REACTOR_OK);

  // connect direct success
  state.type = 1;
  mock_connect = 1;
  mock_connect_errno = 0;
  mock_connect_return = 0;
  e = reactor_tcp_open_addrinfo(&state.client, client_event, &state, r.addrinfo, REACTOR_TCP_FLAG_CLIENT);
  assert_int_equal(e, REACTOR_OK);
  reactor_tcp_close(&state.client);

  reactor_resolver_close(&r);
  reactor_core_destruct();
}

int main()
{
  int e;

  const struct CMUnitTest tests[] = {
    cmocka_unit_test(core)
  };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
