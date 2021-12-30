#include <reactor.h>

static void callback(reactor_event *event)
{
  server_transaction_text((server_transaction *) event->data, data_string("Hello, World!"));
}

int main()
{
  server s1, s2;

  reactor_construct();
  server_construct(&s1, callback, NULL);
  server_construct(&s2, callback, NULL);
  server_open(&s1, net_socket(net_resolve("127.0.0.1", "80", AF_INET, SOCK_STREAM, AI_PASSIVE)), NULL);
  server_open(&s2, net_socket(net_resolve("127.0.0.1", "443", AF_INET, SOCK_STREAM, AI_PASSIVE)),
              net_ssl_context("certs/cert.pem", "certs/key.pem"));
  reactor_loop();
  server_destruct(&s1);
  server_destruct(&s2);
  reactor_destruct();
}
