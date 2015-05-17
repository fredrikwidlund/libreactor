#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <arpa/inet.h>

#include "reactor.h"
#include "reactor_fd.h"
#include "reactor_signal.h"
#include "reactor_signal_dispatcher.h"
#include "reactor_timer.h"
#include "reactor_socket.h"
#include "reactor_resolver.h"

struct app
{
  reactor          *reactor;
  reactor_timer    *timer;
  reactor_socket   *socket;
  reactor_signal   *signal;
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
  reactor_resolver *s = e->data;
  struct gaicb *ar = &s->ar;
  struct addrinfo *ai;
  struct sockaddr_in *sin;
  char name[256];
  int error;
  
  (void) fprintf(stderr, "[resolver reply %s]\n", ar->ar_name);
  for (ai = ar->ar_result; ai; ai = ai->ai_next)
    {
      if (ai->ai_family == AF_INET && ai->ai_socktype == SOCK_STREAM)
	{
	  sin = (struct sockaddr_in *) ai->ai_addr;
	  (void) fprintf(stderr, "-> %s\n", inet_ntop(AF_INET, &sin->sin_addr, name, sizeof name));
	}
    }

  error = reactor_resolver_delete(s);
  if (error == -1)
    err(1, "reactor_resolver_delete");
}

int main()
{
  struct app app;
  struct reactor_resolver *s;
  int e;

  app.reactor = reactor_new();
  if (!app.reactor)
    err(1, "reactor_new");

  s = reactor_resolver_new(app.reactor, resolver_handler, "www.sunet.se", NULL, &app);
  if (!s)
    err(1, "reactor_resolver_new");
  
  s = reactor_resolver_new(app.reactor, resolver_handler, "www.svd.se", NULL, &app);
  if (!s)
    err(1, "reactor_resolver_new");
  
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
