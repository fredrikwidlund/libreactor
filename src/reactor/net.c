#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <linux/bpf.h>
#include <linux/filter.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <dynamic.h>

#include "net.h"

static core_status net_resolve_handler(core_event *event)
{
  net_resolve_job *job = event->state;

  switch (event->type)
  {
  case POOL_REQUEST:
    net_resolve(job->host, job->serv, job->family, job->type, job->flags, &job->addrinfo);
    break;
  case POOL_REPLY:
    (void) core_dispatch(&job->user, 0, (uintptr_t) job);
    free(job->host);
    free(job->serv);
    free(job);
    break;
  }

  return CORE_OK;
}

core_id net_resolve_async(pool *pool, core_callback *callback, void *state, char *host, char *serv, int family, int type,
                          int flags)
{
  net_resolve_job *job;

  job = malloc(sizeof *job);
  *job = (net_resolve_job) {
      .pool = pool,
      .user = {.callback = callback, .state = state},
      .host = strdup(host),
      .serv = strdup(serv),
      .family = family,
      .type = type,
      .flags = flags};
  job->id = pool_enqueue(pool, net_resolve_handler, job);
  return (core_id) job;
}

void net_resolve(char *host, char *serv, int family, int type, int flags, struct addrinfo **addrinfo)
{
  struct addrinfo hints = {.ai_family = family, .ai_socktype = type, .ai_flags = flags};
  int e;

  *addrinfo = NULL;
  e = getaddrinfo(host, serv, &hints, addrinfo);
  if (e != 0 && *addrinfo)
  {
    freeaddrinfo(*addrinfo);
    *addrinfo = NULL;
  }
}

int net_server(struct addrinfo *addrinfo, int flags)
{
  int fd, e = 0;

  if (!addrinfo)
    return -1;

  flags = flags ? flags : NET_SERVER_DEFAULT;
  fd = socket(addrinfo->ai_family, addrinfo->ai_socktype | (flags & NET_FLAG_NONBLOCK ? SOCK_NONBLOCK : 0),
              addrinfo->ai_protocol);
  if (fd == -1)
    return -1;

  if (e == 0 && flags & NET_FLAG_REUSE)
    e = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int));
  if (e == 0 && flags & NET_FLAG_REUSE)
    e = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int));
  if (e == 0 && flags & NET_FLAG_NODELAY)
    e = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (int[]) {1}, sizeof(int));
  if (e == 0 && flags & NET_FLAG_QUICKACK)
    e = setsockopt(fd, IPPROTO_TCP, TCP_QUICKACK, (int[]) {1}, sizeof(int));
  if (e == 0)
    e = bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
  if (e == 0 && addrinfo->ai_socktype == SOCK_STREAM)
    e = listen(fd, -1);
  if (e == 0)
    return fd;

  (void) close(fd);
  return -1;
}

int net_server_filter(int fd, int mod)
{
  struct sock_filter filter[] =
      {
          {BPF_LD | BPF_W | BPF_ABS, 0, 0, SKF_AD_OFF + SKF_AD_CPU}, // A = #cpu
          {BPF_ALU | BPF_MOD | BPF_K, 0, 0, mod},                    // A = A % group_size
          {BPF_RET | BPF_A, 0, 0, 0}                                 // return A
      };
  struct sock_fprog prog = {.len = 3, .filter = filter};
  return setsockopt(fd, SOL_SOCKET, SO_ATTACH_REUSEPORT_CBPF, &prog, sizeof(prog));
}
