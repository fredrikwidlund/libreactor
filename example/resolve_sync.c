#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <err.h>

#include "reactor.h"

static void resolve(char *ai_host, char *ai_serv, int family, int type, int flags)
{
  struct addrinfo *ai;
  char ni_host[NI_MAXHOST], ni_serv[NI_MAXSERV];
  int e;

  net_resolve(ai_host, ai_serv, family, type, flags, &ai);
  if (ai)
  {
    e = getnameinfo(ai->ai_addr, ai->ai_addrlen, ni_host, NI_MAXHOST, ni_serv, NI_MAXSERV,
                    NI_NUMERICHOST | NI_NUMERICSERV);
    if (e != 0)
      err(1, "getnameinfo");
    (void) fprintf(stdout, "%s:%s (family %d, type %d, flags 0x%04x) resolved to node %s service %s\n",
                   ai_host, ai_serv, family, type, flags, ni_host, ni_serv);
  }
  else
    (void) fprintf(stdout, "%s:%s (family %d, type %d, flags 0x%04x) failed to resolv\n",
                   ai_host, ai_serv, family, type, flags);
  freeaddrinfo(ai);
}

int main()
{
  resolve("127.0.0.1", "80", 0, 0, AI_NUMERICHOST | AI_NUMERICSERV);
  resolve("localhost", "http", 0, 0, AI_NUMERICHOST | AI_NUMERICSERV);
  resolve("localhost", "http", 0, 0, 0);
  resolve("localhost", "http", AF_INET, 0, 0);
  resolve("ipv6.google.com", "https", 0, 0, 0);
  resolve("does.not.exist", "https", 0, 0, 0);
  resolve("does.not.exist", "xxx", -1, -1, -1);
}
