#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <limits.h>
#include <sys/epoll.h>
#include <netinet/in.h>

#include <dynamic.h>
#include <reactor.h>

static core_status server_fatal_next(core_event *event)
{
  server *server = event->state;

  server->next = 0;
  return core_dispatch(&server->user, SERVER_ERROR, 0);
}

static void server_fatal(server *server)
{
  if (server->next == 0)
    server->next = core_next(NULL, server_fatal_next, server);
}

static core_status server_session_read(server_session *session)
{
  server_context context;
  segment data;
  size_t offset;
  ssize_t n;
  core_status e;

  context.session = session;
  data = stream_read(&session->stream);
  for (offset = 0; offset < data.size; offset += n)
    {
      n = http_request_read(&context.request, segment_offset(data, offset));
      if (dynamic_unlikely(n == 0))
        break;

      if (dynamic_unlikely(n == -1))
        {
          stream_destruct(&session->stream);
          list_erase(session, NULL);
          return CORE_ABORT;
        }

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
  server *server = event->state;

  if (event->type != TIMER_ALARM)
    {
      server_fatal(server);
      return CORE_ABORT;
    }

  http_date(1);
  return CORE_OK;
}

static core_status server_fd_handler(core_event *event)
{
  server *server = event->state;
  server_session *session;
  int fd;

  if (event->data != EPOLLIN)
    {
      server_fatal(server);
      return CORE_ABORT;
    }

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

static int server_fd_bind(server *server, uint32_t ip, uint16_t port)
{
  struct sockaddr_in sin = {0};
  int e;

  e = setsockopt(server->fd, SOL_SOCKET, SO_REUSEPORT, (int[]){1}, sizeof(int));
  if (e == -1)
    return -1;
  e = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, (int[]){1}, sizeof(int));
  if (e == -1)
    return -1;

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = htonl(ip);
  sin.sin_port = htons(port);
  e = bind(server->fd, (struct sockaddr *) &sin, sizeof sin);
  if (e == -1)
    return -1;

  return listen(server->fd, INT_MAX);
}

void server_construct(server *server, core_callback *callback, void *state)
{
  *server = (struct server) {.user = {.callback = callback, .state = state}, .fd = -1};
  timer_construct(&server->timer, server_timer_handler, server);
  list_construct(&server->sessions);
}

void server_open(server *server, uint32_t ip, uint16_t port)
{
  int e;

  server->fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server->fd == -1)
    {
      server_fatal(server);
      return;
    }

  e = server_fd_bind(server, ip, port);
  if (e == -1)
    {
      (void) close(server->fd);
      server->fd = -1;
      server_fatal(server);
      return;
    }

  core_add(NULL, server_fd_handler, server, server->fd, EPOLLIN | EPOLLET);
  timer_set(&server->timer, 0, 1000000000);
}

void server_close(server *server)
{
  server_session *session;

  list_foreach(&server->sessions, session)
    stream_destruct(&session->stream);
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
  server_close(server);
  list_destruct(&server->sessions, NULL);
  timer_destruct(&server->timer);
}

void server_ok(server_context *context, segment type, segment data)
{
  http_response response;

  http_response_ok(&response, type, data);
  http_response_write(&response, stream_allocate(&context->session->stream, http_response_size(&response)));
}

