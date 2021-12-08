#include <unistd.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <assert.h>

#include "net.h"

struct addrinfo *net_resolve(char *host, char *serv, int family, int type, int flags)
{
  struct addrinfo *addrinfo = NULL, hints = {.ai_family = family, .ai_socktype = type, .ai_flags = flags};
  int e;

  e = getaddrinfo(host, serv, &hints, &addrinfo);
  if (e != 0)
  {
    freeaddrinfo(addrinfo);
    addrinfo = NULL;
  }
  return addrinfo;
}

int net_socket(struct addrinfo *addrinfo)
{
  int fd, e;

  if (!addrinfo)
    return -1;
  fd = socket(addrinfo->ai_family, addrinfo->ai_socktype | SOCK_NONBLOCK, addrinfo->ai_protocol);
  if (addrinfo->ai_flags & AI_PASSIVE)
  {
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int));
    (void) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int));
    e = bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (e == -1)
    {
      e = close(fd);
      assert(e == 0);
      fd = -1;
    }
    else
    {
      e = listen(fd, INT_MAX);
      assert(e == 0);
    }
  }
  else
    (void) connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);

  freeaddrinfo(addrinfo);
  return fd;
}

SSL_CTX *net_ssl_context(char *certificate, char *private_key)
{
  SSL_CTX *ctx;
  int e;

  ctx = SSL_CTX_new(TLS_server_method());
  assert(ctx != NULL);
  e = SSL_CTX_use_certificate_file(ctx, certificate, SSL_FILETYPE_PEM);
  if (e != 1)
  {
    SSL_CTX_free(ctx);
    return NULL;
  }
  e = SSL_CTX_use_PrivateKey_file(ctx, private_key, SSL_FILETYPE_PEM);
  if (e != 1)
  {
    SSL_CTX_free(ctx);
    return NULL;
  }
  return ctx;
}
