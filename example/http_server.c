#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <err.h>

#include <dynamic.h>
#include <reactor.h>

typedef struct client client;
typedef struct server server;

struct client
{
  reactor_http   http;
  server        *server;
};

struct server
{
  reactor_tcp    tcp;
  reactor_timer  timer;
};

static char date[] = "Thu, 01 Jan 1970 00:00:00 GMT";

int timer_event(void *state, int type, void *data)
{
  (void) state;
  (void) data;
  if (type != REACTOR_TIMER_EVENT_CALL)
    err(1, "timer");

  reactor_http_date(date);
  return REACTOR_OK;
}

int client_stream_event(void *state, int type, void *data)
{
  client *client = state;
  reactor_http_request *request;
  struct iovec *iov;

  switch (type)
    {
    case REACTOR_HTTP_EVENT_REQUEST_HEADER:
      request = data;
      (void) fprintf(stderr, "[request] %.*s %.*s\n",
                     (int) request->method.iov_len, (char *) request->method.iov_base,
                     (int) request->path.iov_len, (char *) request->path.iov_base);
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_REQUEST_DATA:
      iov = data;
      (void) fprintf(stderr, "[data] size %lu\n", iov->iov_len);
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_REQUEST_END:
      (void) fprintf(stderr, "[end]\n");
      reactor_http_send_response(&client->http, (reactor_http_response[]){{
            .version = 1,
            .status = 200,
            .reason = {"OK", 2},
            .headers = 4,
            .header[0] = {{"Server", 6}, {"libreactor", 10}},
            .header[1] = {{"Date", 4}, {date, strlen(date)}},
            .header[2] = {{"Content-Type", 12}, {"text/plain", 10}},
            .header[3] = {{"Content-Length", 14}, {"13", 2}},
            .body = {"Hello, World!", 13}
          }});
      return REACTOR_OK;
    default:
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

}

int client_event(void *state, int type, void *data)
{
  client *client = state;
  reactor_http_request *request = data;
  char *body, content_length[11];
  size_t size;

  if (reactor_unlikely(type != REACTOR_HTTP_EVENT_REQUEST))
    {
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

  if (request->method.iov_len == 4 && strncmp(request->method.iov_base, "QUIT", 4) == 0)
    {
      reactor_timer_close(&client->server->timer);
      reactor_tcp_close(&client->server->tcp);
      reactor_http_close(&client->http);
      free(client);
      return REACTOR_ABORT;
    }

  body = "Hello, World!";
  size = strlen(body);
  reactor_util_u32toa(size, content_length);
  reactor_http_send_response(&client->http, (reactor_http_response[]){{
        .version = 1,
        .status = 200,
        .reason = {"OK", 2},
        .headers = 4,
        .header[0] = {{"Server", 6}, {"libreactor", 10}},
        .header[1] = {{"Date", 4}, {date, strlen(date)}},
        .header[2] = {{"Content-Type", 12}, {"text/plain", 10}},
        .header[3] = {{"Content-Length", 14}, {content_length, strlen(content_length)}},
        .body = {body, size}
      }});
  return REACTOR_OK;
}

int server_event(void *state, int type, void *data)
{
  server *server = state;
  client *client;

  if (type != REACTOR_TCP_EVENT_ACCEPT)
    err(1, "tcp");

  client = malloc(sizeof *client);
  if (!client)
    abort();
  client->server = server;
  //reactor_http_open(&client->http, client_event, client, *(int *) data,
  //                  REACTOR_HTTP_FLAG_SERVER);
  reactor_http_open(&client->http, client_stream_event, client, *(int *) data,
                    REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM);
  return REACTOR_OK;
}

int main(int argc, char **argv)
{
  server server;
  int e;

  if (argc != 3)
    err(1, "usage");

  reactor_core_construct();
  reactor_http_date(date);
  e = reactor_timer_open(&server.timer, timer_event, &server, 1000000000, 1000000000);
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_timer_open");

  e = reactor_tcp_open(&server.tcp, server_event, &server, argv[1], argv[2], REACTOR_TCP_FLAG_SERVER);
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_tcp_open");

  e = reactor_core_run();
  if (e ==  REACTOR_ERROR)
    err(1, "reactor_core_run");

  reactor_core_destruct();
}
