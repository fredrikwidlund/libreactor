#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include "reactor.h"
#include "reactor_fd.h"
#include "reactor_signal.h"
#include "reactor_timer.h"
#include "reactor_socket.h"
#include "reactor_resolver.h"

struct app
{
  reactor          *reactor;
  reactor_timer    *timer;
  reactor_socket   *socket;
  reactor_signal   *signal;
  reactor_resolver *resolver;
};

void socket_handler(reactor_event *e)
{
  reactor_data *data = e->data;
  struct app *app = e->call->data;
  
  switch (e->type)
    {
    case REACTOR_SOCKET_CLOSE:
      (void) fprintf(stderr, "[socket close]\n");
      (void) reactor_socket_delete(app->socket);
      app->socket = NULL;
      break;
    case REACTOR_SOCKET_DATA:
      (void) fprintf(stderr, "[socket data] %.*s\n", (int) data->size - 1, data->base);
      break;
    }
}

void timer_handler(reactor_event *e)
{
  (void) fprintf(stderr, "[timer timeout]\n");
}

void signal_handler(reactor_event *e)
{
  struct signalfd_siginfo *fdsi = e->data;
  struct app *app = e->call->data;

  (void) fprintf(stderr, "[signal SIGINT raised, pid %d, int %d, ptr %p\n", fdsi->ssi_pid, fdsi->ssi_int, (void *) fdsi->ssi_ptr);
  reactor_halt(app->reactor);
}

void resolver_handler(reactor_event *e)
{
  (void) fprintf(stderr, "[resolver]\n");
}

int main()
{
  struct app app;
  int e, i;
  
  app.reactor = reactor_new();
  if (!app.reactor)
    err(1, "reactor_new");

  app.resolver = reactor_resolver_new(app.reactor, resolver_handler, &app);
  if (!app.resolver)
    err(1, "reactor_resolver_new");

  e = reactor_resolver_lookup(app.resolver, "www.sunet.se", NULL);
  if (e == -1)
    err(1, "reactor_resolver_lookup");
  
  app.signal = reactor_signal_new(app.reactor, signal_handler, SIGINT, &app);
  if (!app.signal)
    err(1, "reactor_signal_new");
  
  app.socket = reactor_socket_new(app.reactor, socket_handler, STDIN_FILENO, &app);
  if (!app.socket)
    err(1, "reactor_socket_new");

  app.timer = reactor_timer_new(app.reactor, timer_handler, 1000000000, &app);
  if (!app.timer)
    err(1, "reactor_timer_new");
  
  e = reactor_run(app.reactor);
  if (e == -1)
    err(1, "reactor_run");
  
  if (app.socket)
    {
      e = reactor_socket_delete(app.socket);
      if (e == -1)
	err(1, "reactor_socket_delete");
    }

  e = reactor_delete(app.reactor);
  if (e == -1)
    err(1, "reactor_delete");
}
