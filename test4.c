#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
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
#include "reactor_client.h"

void client_handler(reactor_event *e)
{
  switch (e->type)
    {
    case REACTOR_CLIENT_CONNECTED:
      printf("[connected]\n");
      break;
    default:
      break; 
    }
}

int main()
{
  reactor *r;
  reactor_client *c;
  
  assert(r = reactor_new());
  assert(c = reactor_client_new(r, client_handler, SOCK_STREAM, "localhost", "http", c));
  assert(reactor_run(r) == 0);  
  assert(reactor_client_delete(c) == 0);
  assert(reactor_delete(r) == 0);
  sleep(1);
}
