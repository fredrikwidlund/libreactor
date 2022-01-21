#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cmocka.h>

#include "reactor.h"

static void test_net(void **state)
{
  int fd;
  SSL_CTX *ctx;

  (void) state;

  assert_int_equal(chdir(DATADIR), 0);

  /* should work */
  fd = net_socket(net_resolve("127.0.0.1", "0", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_true(fd >= 0);
  close(fd);

  fd = net_socket(net_resolve("127.0.0.1", "0", AF_INET, SOCK_STREAM, 0));
  assert_true(fd >= 0);
  close(fd);

  /* getaddrinfo should return error  */
  fd = net_socket(net_resolve("0.0.0.0", "wrongportname", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_int_equal(fd, -1);

  /* bind should fail */
  fd = net_socket(net_resolve("1.2.3.4", "5", AF_INET, SOCK_STREAM, AI_PASSIVE));
  assert_int_equal(fd, -1);

  assert_true(net_ssl_server("/", "/", NULL) == NULL);
  assert_true(net_ssl_server("test/data/cert.pem", "/", NULL) == NULL);
  assert_true(net_ssl_server("/", "test/data/key.pem", NULL) == NULL);
  ctx = net_ssl_server("test/files/cert.pem", "test/files/key.pem", NULL);
  assert_true(ctx);
  SSL_CTX_free(ctx);

  ctx = net_ssl_server("test/files/cert.pem", "test/files/no.pem", NULL);
  assert_false(ctx);

}

int main()
{
  const struct CMUnitTest tests[] =
      {
          cmocka_unit_test(test_net)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
