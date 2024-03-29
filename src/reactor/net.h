#ifndef REACTOR_NET_H_INCLUDED
#define REACTOR_NET_H_INCLUDED

#include <netdb.h>
#include <openssl/ssl.h>

#include "reactor.h"

struct addrinfo *net_resolve(char *, char *, int, int, int);
int              net_socket(struct addrinfo *);
SSL_CTX         *net_ssl_server_context(char *, char *);

#endif /* REACTOR_NET_H_INCLUDED */
