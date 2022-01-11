#include <stdio.h>
#include <netdb.h>
#include <assert.h>

#include <reactor.h>

static void callback(reactor_event *event)
{
  resolver *resolver = event->state;
  struct addrinfo *addrinfo;
  char host[NI_MAXHOST], service[NI_MAXSERV];
  int e;

  switch (event->type)
  {
  case RESOLVER_SUCCESS:
    addrinfo = (struct addrinfo *) event->data;
    do
    {
      e = getnameinfo(addrinfo->ai_addr, addrinfo->ai_addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
      assert(e == 0);
      (void) printf("%s resolved to %s\n", resolver->host, host);
      addrinfo = addrinfo->ai_next;
    }
    while (addrinfo);
    break;
  case RESOLVER_FAILURE:
    (void) printf("%s did not resolve\n", resolver->host);
    break;
  }
}

int main(int argc, char **argv)
{
  resolver r[argc - 1];
  int i;

  reactor_construct();
  for (i = 0; i < argc - 1; i++)
  {
    resolver_construct(&r[i], callback, &r[i]);
    resolver_lookup(&r[i], argv[i + 1], NULL, 0, SOCK_STREAM, 0, 0);
  }
  reactor_loop();
  for (i = 0; i < argc - 1; i++)
    resolver_destruct(&r[i]);
  reactor_destruct();
}
