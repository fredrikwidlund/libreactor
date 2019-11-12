#ifndef REACTOR_REACTOR_NET_H_INCLUDED
#define REACTOR_REACTOR_NET_H_INCLUDED

#define REACTOR_NET_OPTION_DEFAULTS (REACTOR_NET_OPTION_REUSEPORT)

enum reactor_net_events
{
 REACTOR_NET_EVENT_ERROR,
 REACTOR_NET_EVENT_ACCEPT,
 REACTOR_NET_EVENT_CONNECT
};

enum reactor_net_options
{
 REACTOR_NET_OPTION_REUSEPORT = 0x01,
 REACTOR_NET_OPTION_REUSEADDR = 0x02,
 REACTOR_NET_OPTION_PASSIVE   = 0x04
};

typedef enum reactor_net_options reactor_net_options;
typedef struct reactor_net      reactor_net;

struct reactor_net
{
  reactor_user         user;
  reactor_fd           fd;
  reactor_id           resolver_job;
  reactor_net_options  options;
};

void           reactor_net_construct(reactor_net *, reactor_user_callback *, void *);
void           reactor_net_destruct(reactor_net *);
void           reactor_net_reset(reactor_net *);
void           reactor_net_set(reactor_net *, reactor_net_options);
void           reactor_net_clear(reactor_net *, reactor_net_options);
reactor_status reactor_net_bind(reactor_net *, char *, char *);
reactor_status reactor_net_connect(reactor_net *, char *, char *);

#endif /* REACTOR_REACTOR_NET_H_INCLUDED */
