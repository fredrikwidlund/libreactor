#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <assert.h>
#include <err.h>

#include <dynamic.h>

#include "reactor_core.h"

static const char reply[] =
  "HTTP/1.0 200 OK\r\n"
  "Content-Length: 4\r\n"
  "Content-Type: plain/html\r\n"
  "\r\n"
  "test";

void server_event(void *state, int type, void *data)
{
  char buf[65536];
  int c, *fd;
  ssize_t n;

  (void) state;
  if (type & REACTOR_DESC_READ)
    {
      fd = data;
      c = accept(*fd, NULL, NULL);
      if (c == -1)
        err(1, "accept");
      n = recv(c, buf, sizeof buf, MSG_DONTWAIT);
      if (n > 0)
        {
          n = send(c, reply, sizeof reply - 1, MSG_DONTWAIT);
          if (n != sizeof reply - 1)
            err(1, "send");
        }

      if (n == -1)
        err(1, "recv");
      (void) close(c);
    }
}

int main()
{
  reactor_desc desc;
  int s;

  reactor_core_open();

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s != -1);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int)) == 0);
  assert(setsockopt(s, IPPROTO_TCP, TCP_DEFER_ACCEPT, (int[]){1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) (struct sockaddr_in[])
              {{.sin_zero = 0, .sin_addr = 0, .sin_family = AF_INET, .sin_port = htons(80)}},
              sizeof(struct sockaddr_in)) == 0);
  assert(listen(s, -1) == 0);
  reactor_desc_init(&desc, server_event, &desc);
  reactor_desc_open(&desc, s);
  assert(reactor_core_run() == 0);

  reactor_core_close();
}
