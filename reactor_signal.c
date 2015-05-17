#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>

#include "reactor.h"
#include "reactor_signal.h"

reactor_signal *reactor_signal_new(reactor *r, reactor_handler *h, int sig, void *user)
{
  reactor_signal *s;
  int e;
  
  s = malloc(sizeof *s);
  e = reactor_signal_construct(r, s, h, sig, user);
  if (e == -1)
    {
      (void) reactor_signal_destruct(s);
      return NULL;
    }
  
  return s;
}

int reactor_signal_construct(reactor *r, reactor_signal *s, reactor_handler *h, int sig, void *data)
{
  sigset_t mask;
  int e, fd;

  sigemptyset(&mask);
  sigaddset(&mask, sig);

  e = sigprocmask(SIG_BLOCK, &mask, NULL);
  if (e == -1)
    return -1;
  
  fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (fd == -1)
    return -1;

  *s = (reactor_signal) {.user = {.handler = h, .data = data},
			 .reactor = r, .descriptor = fd,
			 .ev = {.events = EPOLLIN | EPOLLET, .data.ptr = &s->main},
			 .main = {.handler = reactor_signal_handler, .data = s}};

  return epoll_ctl(r->epollfd, EPOLL_CTL_ADD, fd, &s->ev);
}

int reactor_signal_destruct(reactor_signal *s)
{
  int e;

  if (s->descriptor >= 0)
    {
      e = epoll_ctl(s->reactor->epollfd, EPOLL_CTL_DEL, s->descriptor, &s->ev);
      if (e == -1)
	return -1;

      e = close(s->descriptor);
      if (e == -1)
	return -1;
      
      s->descriptor = -1;
    }
  
  return 0;
}

int reactor_signal_delete(reactor_signal *s)
{
  int e;

  e = reactor_signal_destruct(s);
  if (e == -1)
    return -1;
  
  free(s);
  return 0;
}

void reactor_signal_handler(reactor_event *e)
{
  reactor_signal *s = e->call->data;
  struct signalfd_siginfo fdsi;
  ssize_t n;

  while (1)
    {
      n = read(s->descriptor, &fdsi, sizeof fdsi);
      if (n != sizeof fdsi)
	break;

      reactor_dispatch(&s->user, REACTOR_SIGNAL_RAISED, &fdsi);
    }
}

int reactor_signal_descriptor(reactor_signal *s)
{
  return s->descriptor;
}
