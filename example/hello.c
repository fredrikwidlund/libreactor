#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <dynamic.h>
#include <reactor.h>

void hello(__attribute__((unused)) void *state, __attribute__((unused)) int type, void *data)
{
  reactor_http_server_respond_text(((reactor_http_server_context *) data)->session, "Hello, World!");
}

int main()
{
  reactor_http_server server;

  reactor_core_construct();
  reactor_http_server_open(&server, NULL, &server, "127.0.0.1", "80");
  reactor_http_server_route(&server, hello, NULL, "GET", "/");
  reactor_core_run();
  reactor_core_destruct();
}
