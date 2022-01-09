#include <string.h>
#include <reactor.h>

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

static void callback(reactor_event *event)
{
  server *server = event->state;
  server_request *request = (server_request *) event->data;
  struct task *task;

  if (data_equal(request->target, data_string("/hello")))
  {
    server_ok(request, data_string("text/plain"), data_string("Hello, World!"));
  }
  else if (data_equal(request->target, data_string("/wait")))
  {
    task = malloc(sizeof *task);
    task->request = request;
    timer_construct(&task->timer, timeout, task);
    timer_set(&task->timer, 5000000000, 0);
  }
  else if (data_equal(request->target, data_string("/close")))
  {
    server_close(request);
    server_release(request);
  }
  else if (data_equal(request->target, data_string("/shutdown")))
  {
    server_shutdown(server);
    server_release(request);
  }
  else
    server_not_found(request);
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
