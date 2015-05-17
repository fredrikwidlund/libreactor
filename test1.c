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

#include "buffer.h"
#include "vector.h"
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
  reactor_data *message = e->message;
  struct app *app = e->receiver->object;
  
  switch (e->type)
    {
    case REACTOR_SOCKET_CLOSE:
      (void) fprintf(stderr, "[socket close]\n");
      (void) reactor_socket_delete(app->socket);
      app->socket = NULL;
      break;
    case REACTOR_SOCKET_DATA:
      (void) fprintf(stderr, "[socket data] %.*s\n", (int) message->size - 1, message->base);
      break;
    }
}

void timer_handler(reactor_event *e)
{
  (void) fprintf(stderr, "[timer timeout]\n");
}

void signal_handler(reactor_event *e)
{
  struct signalfd_siginfo *fdsi = e->message;
  struct app *app = e->receiver->object;

  (void) fprintf(stderr, "[signal SIGINT raised, pid %d, int %d, ptr %p\n", fdsi->ssi_pid, fdsi->ssi_int, (void *) fdsi->ssi_ptr);
  reactor_halt(app->reactor);
}

void resolver_handler(reactor_event *e)
{
  reactor_resolver *s = e->sender;
  int i;
  struct addrinfo *ai;
  struct sockaddr_in *sin;
  struct gaicb *ar;
  char name[256];
  int found;

  (void) fprintf(stderr, "[resolver reply]\n");
  for (i = 0; i < s->nitems; i ++)
    {
      ar = s->list[i];
      (void) fprintf(stderr, "%s\n", ar->ar_name);
      found = 0;
      for (ai = ar->ar_result; ai; ai = ai->ai_next)
	{
	  if (ai->ai_family == AF_INET && ai->ai_socktype == SOCK_STREAM)
	    {
	      sin = (struct sockaddr_in *) ai->ai_addr;
	      (void) fprintf(stderr, "-> %s\n", inet_ntop(AF_INET, &sin->sin_addr, name, sizeof name));
	      found = 1;
	    }
	}
      if (!found)
	(void) fprintf(stderr, "-> (failed)\n");
    }
}

void resolver_handler_simple(reactor_event *e)
{
  reactor_resolver *s = e->sender;
  struct addrinfo *ai = s->list[0]->ar_result;
  char name[256];

  if (ai)
    {
      inet_ntop(AF_INET, &((struct sockaddr_in *) ai->ai_addr)->sin_addr, name, sizeof name);
      (void) fprintf(stderr, "-> %s\n", name);
    }
  
  (void) reactor_resolver_delete(s);  
}

int main()
{
  struct app app;
  struct reactor_resolver *s;
  int e;

  app.reactor = reactor_new();
  if (!app.reactor)
    err(1, "reactor_new");

  /*
  s = reactor_resolver_new_simple(app.reactor, resolver_handler_simple, "www.svtplay.se", "http", &app);
  if (!s)
    err(1, "reactor_resolver_new");
  */

  s = reactor_resolver_new(app.reactor,
			   (struct gaicb *[]){
			     (struct gaicb[]){{.ar_name = "www.sunet.se"}},
			     (struct gaicb[]){{.ar_name = "www.dontexist.blah"}}
			   }, 2, &app, resolver_handler, NULL);
  if (!s)
    err(1, "reactor_resolver_new");

  app.signal = reactor_signal_new(app.reactor, SIGINT, &app, signal_handler, NULL);
  if (!app.signal)
    err(1, "reactor_signal_new");
  
  app.socket = reactor_socket_new(app.reactor, STDIN_FILENO, &app, socket_handler, NULL);
  if (!app.socket)
    err(1, "reactor_socket_new");
  
  app.timer = reactor_timer_new(app.reactor, 1000000000, &app, timer_handler, NULL);
  if (!app.timer)
    err(1, "reactor_timer_new");

  e = reactor_run(app.reactor);
  if (e == -1)
    err(1, "reactor_run");

  e = reactor_resolver_delete(s);
  if (e == -1)
    err(1, "reactor_resolver_delete");

  reactor_timer_delete(app.timer);
  reactor_signal_delete(app.signal);
  
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
