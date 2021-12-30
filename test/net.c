#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void basic(void **state)
{
  int fd;
  SSL_CTX *ctx;

  (void) state;
  /* should work */
  fd = net_socket(net_resolve("127.0.0.1", "0", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_true(fd >= 0);
  close(fd);

  /* getaddrinfo should return error  */
  fd = net_socket(net_resolve("0.0.0.0", "wrongportname", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_int_equal(fd, -1);

  /* bind should fail */
  fd = net_socket(net_resolve("1.2.3.4", "5", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_int_equal(fd, -1);

  assert_true(net_ssl_context("/", "/") == NULL);
  assert_true(net_ssl_context("test/data/cert.pem", "/") == NULL);
  assert_true(net_ssl_context("/", "test/data/key.pem") == NULL);
  ctx = net_ssl_context("test/data/cert.pem", "test/data/key.pem");
  assert_true(ctx);
  SSL_CTX_free(ctx);
}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(basic)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
