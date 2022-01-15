#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <netinet/tcp.h>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/rand.h>

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

SSL_CTX *net_ssl_client(int verify)
{
  SSL_CTX *ssl_ctx;

  ssl_ctx = SSL_CTX_new(TLS_client_method());
  SSL_CTX_set_default_verify_paths(ssl_ctx);
  SSL_CTX_set_verify(ssl_ctx, verify ? SSL_VERIFY_PEER : SSL_VERIFY_NONE, NULL);
  return ssl_ctx;
}

SSL_CTX *net_ssl_server(char *certificate, char *private_key, char *certificate_chain)
{
  SSL_CTX *ssl_ctx;
  int e;

  ssl_ctx = SSL_CTX_new(TLS_server_method());
  assert(ssl_ctx != NULL);
  SSL_CTX_set_default_verify_paths(ssl_ctx);
  e = SSL_CTX_set_min_proto_version(ssl_ctx, TLS1_2_VERSION);
  assert(e == 1);
  SSL_CTX_set_session_cache_mode(ssl_ctx, SSL_SESS_CACHE_OFF);
  e = SSL_CTX_set_cipher_list(ssl_ctx,
                              // tls 1.3
                              "TLS_AES_128_GCM_SHA256:"
                              "TLS_AES_256_GCM_SHA384:"
                              "TLS_CHACHA20_POLY1305_SHA256:"
                              // tls 1.2
                              "ECDHE-ECDSA-AES128-GCM-SHA256:"
                              "ECDHE-RSA-AES128-GCM-SHA256:"
                              "ECDHE-ECDSA-AES256-GCM-SHA384:"
                              "ECDHE-RSA-AES256-GCM-SHA384:"
                              "ECDHE-ECDSA-CHACHA20-POLY1305:"
                              "ECDHE-RSA-CHACHA20-POLY1305:"
                              "DHE-RSA-AES128-GCM-SHA256:"
                              "DHE-RSA-AES256-GCM-SHA384");
  assert(e == 1);
  e = SSL_CTX_set1_curves_list(ssl_ctx, "X25519:prime256v1:secp384r1");
  assert(e == 1);

  e = 1;
  if (certificate_chain)
    e = SSL_CTX_use_certificate_chain_file(ssl_ctx, certificate_chain);
  if (e == 1)
    e = SSL_CTX_use_certificate_file(ssl_ctx, certificate, SSL_FILETYPE_PEM);
  if (e == 1)
    e = SSL_CTX_use_PrivateKey_file(ssl_ctx, private_key, SSL_FILETYPE_PEM);
  if (e != 1)
  {
    SSL_CTX_free(ssl_ctx);
    return NULL;
  }

  return ssl_ctx;
}

void net_ssl_free(SSL_CTX *ssl_ctx)
{
  SSL_CTX_free(ssl_ctx);
}
