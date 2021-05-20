#ifndef REACTOR_NET_H_INCLUDED
#define REACTOR_NET_H_INCLUDED

#define NET_SERVER_DEFAULT (NET_FLAG_NONBLOCK | NET_FLAG_REUSE | NET_FLAG_NODELAY)

enum
{
  NET_FLAG_NONE     = 1 << 0,
  NET_FLAG_NONBLOCK = 1 << 1,
  NET_FLAG_REUSE    = 1 << 2,
  NET_FLAG_NODELAY  = 1 << 3,
  NET_FLAG_QUICKACK = 1 << 4
};

typedef struct net_resolve_job net_resolve_job;
struct net_resolve_job
{
  pool            *pool;
  core_id          id;
  core_handler     user;
  char            *host;
  char            *serv;
  int              family;
  int              type;
  int              flags;
  struct addrinfo *addrinfo;
};

void    net_resolve(char *, char *, int, int, int, struct addrinfo **);
core_id net_resolve_async(pool *, core_callback *, void *, char *, char *, int, int, int);
int     net_server(struct addrinfo *, int);
int     net_server_filter(int, int);

#endif /* REACTOR_NET_H_INCLUDED */
