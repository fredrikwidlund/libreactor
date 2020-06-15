#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include <dynamic.h>
#include <reactor.h>

enum
{
  SERVER_ERROR,
  SERVER_REQUEST
};

enum
{
  SERVER_DIRECT = 0x01,
  SERVER_ABORT  = 0x02 // we need someway to signal that a context is no longer valid, could also ref count stream, or remove stream
};

typedef struct server         server;
typedef struct server_context server_context;
typedef struct server_session server_session;

struct server
{
  core_handler    user;
  timer           timer;
  int             fd;
  list            sessions;
};

struct server_context
{
  int             flags;
  server_session *session;
  http_request    request;
};

struct server_session
{
  stream          stream;
  server         *server;
};


static void server_ok_fast(server_context *context, segment type, segment data)
{
  const segment s[] =
    {
     segment_string("HTTP/1.1 200 OK\r\n"
                    "Server: libreactor\r\n"
                    "Date: "), http_date(0), segment_string("\r\n"),
     segment_string("Content-Type: "), type, segment_string("\r\n"),
     segment_string("Content-Length: "), utility_u32_segment(data.size), segment_string("\r\n\r\n"),
     data
    };
  ssize_t n;
  int count = sizeof s / sizeof s[0];

  n = writev(context->session->stream.fd, (const struct iovec *) s, count);
  if (n != 135)
    {
      printf("%ld\n", n);
      abort();
    }
}

void server_ok(server_context *context, segment type, segment data)
{
  http_response response;

  http_response_ok(&response, type, data);
  http_response_write(&response, stream_allocate(&context->session->stream, http_response_size(&response)));
}

static core_status server_session_read(server_session *session)
{
  server_context context;
  segment data;
  size_t offset;
  ssize_t n;
  core_status e;

  context.flags = 0;
  context.session = session;
  data = stream_read(&session->stream);
  for (offset = 0; offset < data.size; offset += n)
    {
      n = http_request_read(&context.request, segment_offset(data, offset));
      if (n <= 0)
        break;

      if (dynamic_likely(offset + n == data.size))
        context.flags |= SERVER_DIRECT;
      e = core_dispatch(&session->server->user, SERVER_REQUEST, (uintptr_t) &context);
      if (dynamic_unlikely(e != CORE_OK))
        return e;
    }

  stream_flush(&session->stream);
  stream_consume(&session->stream, offset);
  return CORE_OK;
}

static core_status server_session_handler(core_event *event)
{
  server_session *session = (server_session *) event->state;

  if (dynamic_likely(event->type == STREAM_READ))
    return server_session_read(session);

  if (event->type == STREAM_FLUSH)
    return CORE_OK;

  stream_destruct(&session->stream);
  list_erase(session, NULL);
  return CORE_ABORT;
}

static core_status server_timer_handler(core_event *event)
{
  (void) event;
  http_date(1);
  return CORE_OK;
}

static core_status server_fd_handler(core_event *event)
{
  server *server = event->state;
  server_session *session;
  int fd;

  if (event->data != EPOLLIN)
    err(1, "server");

  while (1)
    {
      fd = accept4(server->fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
      if (fd == -1)
        break;

      session = list_push_back(&server->sessions, NULL, sizeof *session);
      stream_construct(&session->stream, server_session_handler, session);
      session->server = server;
      stream_open(&session->stream, fd);
    }

  return CORE_OK;
}

void server_construct(server *server, core_callback *callback, void *state)
{
  *server = (struct server) {.user = {.callback = callback, .state = state}, .fd = -1};
  timer_construct(&server->timer, server_timer_handler, server);
  list_construct(&server->sessions);
}

void server_close(server *server)
{
  timer_clear(&server->timer);
  if (server->fd >= 0)
    {
      core_delete(NULL, server->fd);
      (void) close(server->fd);
      server->fd = -1;
    }
}

void server_destruct(server *server)
{
  server_session *session;

  server_close(server);
  list_foreach(&server->sessions, session)
    stream_destruct(&session->stream);
  list_destruct(&server->sessions, NULL);
  timer_destruct(&server->timer);
}

void server_open(server *server, uint32_t ip, uint16_t port)
{
  struct sockaddr_in sin = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(ip), .sin_port = htons(port)};
  int e, fd;

  timer_set(&server->timer, 0, 1000000000);

  fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  (void) setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  (void) setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
  e = bind(fd, (struct sockaddr *) &sin, sizeof sin);
  if (e == -1)
    err(1, "bind");
  (void) listen(fd, INT_MAX);
  server->fd = fd;
  core_add(NULL, server_fd_handler, server, server->fd, EPOLLIN | EPOLLET);
}

/*******/
/* app */
/*******/

static core_status handler(core_event *event)
{
  server_context *context = (server_context *) event->data;

  if (segment_equal(context->request.method, segment_string("QUIT")))
    {
      server_destruct(context->session->server);
      return CORE_ABORT;
    }

  server_ok(context, segment_string("text/plain"), segment_string("Hello, World!"));
  return CORE_OK;
}

int main()
{
  server s;

  signal(SIGPIPE, SIG_IGN);
  core_construct(NULL);
  server_construct(&s, handler, &s);
  server_open(&s, 0, 8080);
  core_loop(NULL);
  core_destruct(NULL);
}
