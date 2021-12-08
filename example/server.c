#include <string.h>
#include <reactor.h>

static void callback(reactor_event *event)
{
  server_transaction *t = (server_transaction *) event->data;

  server_transaction_text(t, string_constant("Hello, World!"));
}

int main()
{
  server server;

  reactor_construct();
  server_construct(&server, callback, &server);
  server_open(&server, net_socket(net_resolve("127.0.0.1", "80", AF_INET, SOCK_STREAM, AI_PASSIVE)), NULL);
  reactor_loop();
  server_destruct(&server);
  reactor_destruct();
}
