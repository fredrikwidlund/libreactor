#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <err.h>

#include "reactor.h"

static core_status resolve_handler(core_event *event)
{
  net_resolve_job *job = (net_resolve_job *) event->data;
  char ni_host[NI_MAXHOST], ni_serv[NI_MAXSERV];
  int e;

  if (job->addrinfo)
  {
    e = getnameinfo(job->addrinfo->ai_addr, job->addrinfo->ai_addrlen, ni_host, NI_MAXHOST, ni_serv, NI_MAXSERV,
                    NI_NUMERICHOST | NI_NUMERICSERV);
    if (e != 0)
      err(1, "getnameinfo");
    (void) fprintf(stdout, "%s:%s (family %d, type %d, flags 0x%04x) resolved to node %s service %s\n",
                   job->host, job->serv, job->family, job->type, job->flags, ni_host, ni_serv);
  }
  else
    (void) fprintf(stdout, "%s:%s (family %d, type %d, flags 0x%04x) failed to resolv\n",
                   job->host, job->serv, job->family, job->type, job->flags);

  freeaddrinfo(job->addrinfo);
  return CORE_OK;
}

int main()
{
  reactor_construct();
  net_resolve_async(NULL, resolve_handler, NULL, "127.0.0.1", "80", 0, 0, AI_NUMERICHOST | AI_NUMERICSERV);
  net_resolve_async(NULL, resolve_handler, NULL, "localhost", "http", 0, 0, AI_NUMERICHOST | AI_NUMERICSERV);
  net_resolve_async(NULL, resolve_handler, NULL, "localhost", "http", 0, 0, 0);
  net_resolve_async(NULL, resolve_handler, NULL, "localhost", "http", AF_INET, 0, 0);
  net_resolve_async(NULL, resolve_handler, NULL, "ipv6.google.com", "https", 0, 0, 0);
  net_resolve_async(NULL, resolve_handler, NULL, "does.not.exist", "https", 0, 0, 0);
  net_resolve_async(NULL, resolve_handler, NULL, "does.not.exist", "xxx", -1, -1, -1);
  reactor_loop();
  reactor_destruct();
}
