#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <setjmp.h>
#include <signal.h>
#include <netdb.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cmocka.h>
#include <dynamic.h>

#include "reactor.h"

int reactor_stream_event(void *, int, void *);

struct iterate_state
{
  reactor_http  http;
  reactor_timer timer;
  int           valid;
  int           fd;
  int           pair[2];
};

static int iterate_timer_event(void *state, int type, void *data)
{
  struct iterate_state *s = state;
  char buffer[16384];
  ssize_t n;

  (void) type;
  (void) data;
  assert_int_equal(type, REACTOR_TIMER_EVENT_CALL);
  n = read(s->fd, buffer, sizeof buffer);
  if (n == 0)
    {
      reactor_timer_close(&s->timer);
      (void) close(s->fd);
      (void) close(s->pair[0]);
      return REACTOR_ABORT;
    }
  (void) write(s->pair[0], buffer, n);
  return REACTOR_OK;
}

static int iterate_http_event(void *state, int type, void *data)
{
  struct iterate_state *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_HTTP_EVENT_REQUEST_HEADER:
    case REACTOR_HTTP_EVENT_REQUEST_DATA:
    case REACTOR_HTTP_EVENT_RESPONSE_HEADER:
    case REACTOR_HTTP_EVENT_RESPONSE_DATA:
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_REQUEST_END:
    case REACTOR_HTTP_EVENT_REQUEST:
    case REACTOR_HTTP_EVENT_RESPONSE:
    case REACTOR_HTTP_EVENT_RESPONSE_END:
      assert_true(s->valid);
      return REACTOR_OK;
    case REACTOR_HTTP_EVENT_ERROR:
      assert_true(!s->valid);
      reactor_http_close(&s->http);
      return REACTOR_ABORT;
    case REACTOR_HTTP_EVENT_CLOSE:
      reactor_http_close(&s->http);
      return REACTOR_ABORT;
    default:
      assert_true(0);
      return REACTOR_ABORT;
    }
}

static void iterate_test(char *path, int valid, int flags)
{
  struct iterate_state s = {0};

  s.valid = valid;
  s.fd = open(path, O_RDONLY);
  assert_true(s.fd >= 0);
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, s.pair), 0);

  assert_int_equal(reactor_core_construct(), REACTOR_OK);
  assert_int_equal(reactor_timer_open(&s.timer, iterate_timer_event, &s, 1000000, 1000000), REACTOR_OK);
  assert_int_equal(reactor_http_open(&s.http, iterate_http_event, &s, s.pair[1], flags), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
}

static void iterate()
{
  char cwd[PATH_MAX];
  DIR *dir;
  struct dirent *d;
  int flags, allow;

  getcwd(cwd, sizeof cwd);
  assert_int_equal(chdir(SRCDIR "test/data/http"), 0);
  dir = opendir(".");
  assert_true(dir != NULL);
  while (1)
    {
      d = readdir(dir);
      if (!d)
        break;
      if (d->d_type != DT_REG || strlen(d->d_name) < 5)
        continue;
      flags = strncmp(d->d_name, "request", strlen("request")) == 0 ?
        REACTOR_HTTP_FLAG_SERVER : REACTOR_HTTP_FLAG_CLIENT;
      allow = strcmp(d->d_name + strlen(d->d_name) - 5, "allow") == 0;
      iterate_test(d->d_name, allow, flags);
      iterate_test(d->d_name, allow, flags | REACTOR_HTTP_FLAG_STREAM);
    }
  (void)closedir(dir);
  assert_int_equal(chdir(cwd), 0);
}

static int transaction_server_event(void *state, int type, void *data)
{
  reactor_http *server = state;
  reactor_http_request *request;
  char date[] = "Thu, 01 Jan 1970 00:00:00 GMT";
  char body[1048576] = {0};

  if (type == REACTOR_HTTP_EVENT_CLOSE)
    {
      reactor_http_close(server);
      return REACTOR_ABORT;
    }

  assert_true(type != REACTOR_HTTP_EVENT_ERROR);
  if (type == REACTOR_HTTP_EVENT_REQUEST ||
      type == REACTOR_HTTP_EVENT_REQUEST_HEADER)
    {
      request = data;
      assert_int_equal(request->version, 1);
      reactor_http_date(date);
      reactor_http_send_response(server, (reactor_http_response[]){{
            .version = 1,
            .status = 200,
            .reason = {"OK", 2},
            .headers = 4,
            .header[0] = {{"Server", 6}, {"libreactor", 10}},
            .header[1] = {{"Date", 4}, {date, strlen(date)}},
            .header[2] = {{"Content-Type", 12}, {"text/plain", 10}},
            .header[3] = {{"Content-Length", 14}, {"1048576", 7}},
            .body = {body, 1048576}
        }});
    }

  return REACTOR_OK;
}

static int transaction_client_event(void *state, int type, void *data)
{
  reactor_http *client = state;
  reactor_http_response *response = data;

  if (type == REACTOR_HTTP_EVENT_RESPONSE ||
      type == REACTOR_HTTP_EVENT_RESPONSE_HEADER)
    assert_int_equal(response->status, 200);

  if (type == REACTOR_HTTP_EVENT_RESPONSE ||
      type == REACTOR_HTTP_EVENT_RESPONSE_END)
    {
      reactor_http_close(client);
      return REACTOR_ABORT;
    }

  return REACTOR_OK;
}

static void transaction()
{
  reactor_http client, server;
  int pair[2];
  char body[1048576] = {0};

  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);

  assert_int_equal(reactor_http_open(&server, transaction_server_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client, transaction_client_event, &client, pair[0],
                                     REACTOR_HTTP_FLAG_CLIENT), REACTOR_OK);
  reactor_http_send_request(&client, (reactor_http_request[]){{
        .method = {"GET", 3},
        .path = {"/", 1},
        .headers = 1,
        .header[0] = {{"Content-Length", 14}, {"1048576", 7}},
        .version = 1,
        .body = {body, 1048576}
      }});
  assert_int_equal(reactor_http_flush(&client), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();

  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  assert_int_equal(reactor_core_construct(), REACTOR_OK);

  assert_int_equal(reactor_http_open(&server, transaction_server_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client, transaction_client_event, &client, pair[0],
                                     REACTOR_HTTP_FLAG_CLIENT | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client, (reactor_http_request[]){{
        .method = {"GET", 3},
        .path = {"/", 1},
        .headers = 1,
        .header[0] = {{"Content-Length", 14}, {"1048576", 7}},
        .version = 1,
        .body = {body, 1048576}
      }});
  assert_int_equal(reactor_http_flush(&client), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);
  reactor_core_destruct();
}

struct corner_state
{
  reactor_http  http;
  int           type_abort;
};

static int corner_event(void *state, int type, void *data)
{
  struct corner_state *s = state;

  (void) data;
  switch (type)
    {
    case REACTOR_HTTP_EVENT_ERROR:
    case REACTOR_HTTP_EVENT_CLOSE:
      reactor_http_close(&s->http);
      return REACTOR_ABORT;
    default:
      if (type == s->type_abort)
        {
          reactor_http_close(&s->http);
          return REACTOR_ABORT;
        }
      return REACTOR_OK;
    }
}

static void corner()
{
  struct corner_state server = {0}, client = {0};
  int pair[2];

  assert_int_equal(reactor_core_construct(), REACTOR_OK);

  // abort on request_header event
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_HEADER;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Content-Length", 14}, {"4", 1}}, .body = {"test", 4}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on request_data event
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_DATA;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Content-Length", 14}, {"4", 1}}, .body = {"test", 4}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on request_end event
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Content-Length", 14}, {"4", 1}}, .body = {"test", 4}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on request_end while streaming last chunk
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}}, .body = {"\0\r\n\r\n", 5}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on request_end while reading last chunk
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}}, .body = {"\0\r\n\r\n", 5}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  server.http.state = -1;
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on request
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}}, .body = {"\0\r\n\r\n", 5}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  server.http.state = -1;
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on response header
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  client.type_abort = REACTOR_HTTP_EVENT_RESPONSE_HEADER;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);

  reactor_http_send_response(&server.http, (reactor_http_response[]){{
        .version = 1, .status = 200, .reason = {"OK", 2}, .headers = 1, .header[0] = {{"Content-Length", 14}, {"4", 1}},
        .body = {"test", 4}
      }});
  assert_int_equal(reactor_http_flush(&server.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on response data
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  client.type_abort = REACTOR_HTTP_EVENT_RESPONSE_DATA;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);

  reactor_http_send_response(&server.http, (reactor_http_response[]){{
        .version = 1, .status = 200, .reason = {"OK", 2}, .headers = 1, .header[0] = {{"Content-Length", 14}, {"4", 1}},
        .body = {"test", 4}
      }});
  assert_int_equal(reactor_http_flush(&server.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // abort on response end
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  client.type_abort = REACTOR_HTTP_EVENT_RESPONSE_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);

  reactor_http_send_response(&server.http, (reactor_http_response[]){{
        .version = 1, .status = 200, .reason = {"OK", 2}, .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}},
        .body = {"\0\r\n\r\n", 5}
      }});
  assert_int_equal(reactor_http_flush(&server.http), REACTOR_OK);
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // trigger http server state data
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_http_send_request(&client.http, (reactor_http_request[]){{.method = {"GET", 3}, .path = {"/", 1},
          .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}}, .body = {"\0\r\n\r\n", 5}}});
  assert_int_equal(reactor_http_flush(&client.http), REACTOR_OK);
  server.http.state = REACTOR_HTTP_STATE_DATA;
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // trigger http client state error
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[0], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);

  reactor_http_send_response(&server.http, (reactor_http_response[]){{
        .version = 1, .status = 200, .reason = {"OK", 2}, .headers = 1, .header[0] = {{"Transfer-Encoding", 17}, {"chunked", 7}},
        .body = {"\0\r\n\r\n", 5}
      }});
  assert_int_equal(reactor_http_flush(&server.http), REACTOR_OK);
  client.http.state = -1;
  assert_int_equal(reactor_core_run(), REACTOR_OK);

  // server stream error streaming
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER | REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_stream_event(&server.http.stream, 0, (int[]){0});
  close(pair[0]);

  // server stream error
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&server.http, corner_event, &server, pair[1],
                                     REACTOR_HTTP_FLAG_SERVER), REACTOR_OK);
  reactor_stream_event(&server.http.stream, 0, (int[]){0});
  close(pair[0]);

  // client stream error
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[1], 0), REACTOR_OK);
  reactor_stream_event(&client.http.stream, 0, (int[]){0});
  close(pair[0]);

  // client stream error while streaming
  assert_int_equal(socketpair(AF_UNIX, SOCK_STREAM, 0, pair), 0);
  server.type_abort = REACTOR_HTTP_EVENT_REQUEST_END;
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, pair[1], REACTOR_HTTP_FLAG_STREAM), REACTOR_OK);
  reactor_stream_event(&client.http.stream, 0, (int[]){0});
  close(pair[0]);

  // open with invalid flags
  assert_int_equal(reactor_http_open(&client.http, corner_event, &client, 0, -1), REACTOR_ERROR);

  reactor_core_destruct();
}

static void header()
{
  struct iovec *iov;

  iov = reactor_http_header_get((reactor_http_header[]){{{"key", 3}, {"value", 5}}}, 1, "abc");
  assert_true(iov == NULL);
  iov = reactor_http_header_get((reactor_http_header[]){{{"key", 3}, {"value", 5}}}, 1, "key");
  assert_true(iov != NULL);
  assert_true(reactor_http_header_value(iov, "value"));
  assert_false(reactor_http_header_value(iov, "abcde"));
}

int main()
{
  int e;
  signal(SIGPIPE, SIG_IGN);
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(transaction),
    cmocka_unit_test(iterate),
    cmocka_unit_test(corner),
    cmocka_unit_test(header)
 };

  e = cmocka_run_group_tests(tests, NULL, NULL);
  (void) close(0);
  (void) close(1);
  (void) close(2);
  return e;
}
