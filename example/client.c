#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <err.h>
#include <assert.h>

#include <openssl/ssl.h>

#include <reactor.h>

enum
{
  CLIENT_SUCCESS,
  CLIENT_FAILURE
};

typedef struct client          client;
typedef struct client_response client_response;

struct client
{
  reactor_handler handler;
  stream          stream;
  bool            request_in_progress;
  bool            request_is_head;
  size_t          chunked_data_processed;
  size_t          body_processed;
  uint64_t        t0;
  uint64_t        t1;
};

struct client_response
{
  int             status_code;
  data            status;
  http_field      fields[16];
  size_t          fields_count;
  data            body;
};

void client_construct(client *, reactor_callback *, void *);
void client_destruct(client *);
void client_open(client *, int, SSL_CTX *);
void client_get(client *, data, data);
void client_close(client *);

static ssize_t client_dechunk(data buffer, size_t *chunked_data_processed, size_t *body_processed, data *body)
{
  data data;
  char *end, *eot;
  size_t size, size_consumed;

  while (1)
  {
    data = data_consume(buffer, *chunked_data_processed);
    end = memchr(data_base(data), '\n', data_size(data));
    if (!end)
      return -2;
    size = strtoul(data_base(data), &eot, 16);
    if (eot == data_base(data))
      return -1;
    end++;
    size_consumed = end - (char *) data_base(data) + size + 2;
    if (size_consumed > data_size(data))
      return -2;
    *chunked_data_processed += size_consumed;
    if (!size)
      break;
    memmove((char *) data_base(buffer) + *body_processed, end, size);
    *body_processed += size;
  }

  *body = data_select(buffer, 0, *body_processed);
  return *chunked_data_processed;
}

static ssize_t client_read_body(data buffer, http_field *fields, size_t fields_count, size_t *chunked_data_processed,
                                size_t *body_processed, data *body)
{
  data encoding = http_field_lookup(fields, fields_count, data_string("Transfer-Encoding"));
  if (!data_empty(encoding) && !data_equal(encoding, data_string("identity")))
    return client_dechunk(buffer, chunked_data_processed, body_processed, body);

  data content_length = http_field_lookup(fields, fields_count, data_string("Content-Length"));
  if (data_empty(content_length))
    return -1;

  *body = data_select(buffer, 0, strtoul(data_base(content_length), NULL, 10));
  return data_size(*body) > data_size(buffer) ? -2 : (ssize_t) data_size(*body);
}

static ssize_t client_read_request_body(data buffer, int request_is_head, int status_code,
                                        http_field *fields, size_t fields_count, size_t *chunked_data_processed,
                                        size_t *body_processed, data *body)
{
  if (request_is_head || (status_code >= 100 && status_code < 200) || status_code == 204 || status_code == 304)
    return 0;
  return client_read_body(buffer, fields, fields_count, chunked_data_processed, body_processed, body);
}

static void client_read(client *client)
{
  client_response response;
  data data;
  ssize_t header_consumed, body_consumed = 0, valid;

  response.fields_count = sizeof response.fields / sizeof response.fields[0];
  data = stream_read(&client->stream);
  header_consumed = http_read_response_data(data, &response.status_code, &response.status,
                                            response.fields, &response.fields_count);
  valid = header_consumed;
  if (valid >= 0)
  {
    body_consumed = client_read_request_body(data_consume(data, header_consumed), client->request_is_head,
                                             response.status_code, response.fields, response.fields_count,
                                             &client->chunked_data_processed, &client->body_processed, &response.body);
    valid = body_consumed;
  }

  switch (valid)
  {
  case -2:
    break;
  case -1:
    client_close(client);
    reactor_dispatch(&client->handler, CLIENT_FAILURE, 0);
    break;
  default:
    stream_consume(&client->stream, header_consumed + body_consumed);
    client->request_in_progress = 0;
    reactor_dispatch(&client->handler, CLIENT_SUCCESS, (uintptr_t) &response);
    break;
  }
}

static void client_callback(reactor_event *event)
{
  client *client = event->state;

  switch (event->type)
  {
  case STREAM_READ:
    client_read(client);
    break;
  case STREAM_CLOSE:
    client_close(client);
    reactor_dispatch(&client->handler, CLIENT_FAILURE, 0);
    break;
  }
}

void client_construct(client *client, reactor_callback *callback, void *state)
{
  *client = (struct client) {0};
  reactor_handler_construct(&client->handler, callback, state);
  stream_construct(&client->stream, client_callback, client);
}

void client_open(client *client, int fd, SSL_CTX *ssl_ctx)
{
  stream_open(&client->stream, fd, STREAM_READ, ssl_ctx);
}

void client_get(client *client, data host, data target)
{
  assert(client->request_in_progress == 0);
  client->request_in_progress = 1;
  client->request_is_head = 0;
  client->chunked_data_processed = 0;
  client->body_processed = 0;

  http_write_request(&client->stream, data_string("GET"), target, host, data_null(), data_null());
  stream_flush(&client->stream);
}

void client_close(client *client)
{
  stream_close(&client->stream);
  client->request_in_progress = 0;
}

void client_destruct(client *client)
{
  stream_close(&client->stream);
}

static void callback(reactor_event *event)
{
  client *client = event->state;
  client_response *response;

  switch (event->type)
  {
  case CLIENT_SUCCESS:
    response = (client_response *) event->data;
    client->t1 = utility_tsc();
    (void) fprintf(stderr, "[status %d, size %lu, time %lu]\n", response->status_code, response->body.size, client->t1 - client->t0);
    (void) fprintf(stdout, "%.*s", (int) response->body.size, (char *) response->body.base);
    client_close(client);
    break;
  case CLIENT_FAILURE:
    (void) fprintf(stderr, "[error: request failed]\n");
    break;
  }
}

int main(int argc, char **argv)
{
  extern char *__progname;
  client client;
  int fd;
  SSL_CTX *ssl_ctx;

  if (argc != 3)
  {
    (void) fprintf(stderr, "usage: %s host target\n", __progname);
    exit(1);
  }

  ssl_ctx = net_ssl_client(1);

  reactor_construct();
  client_construct(&client, callback, &client);

  fd = net_socket(net_resolve(argv[1], "https", AF_INET, SOCK_STREAM, 0));
  if (fd == -1)
    errx(1, "unable to connect");

  client_open(&client, fd, ssl_ctx);
  client.t0 = utility_tsc();
  client_get(&client, data_string(argv[1]), data_string(argv[2]));

  reactor_loop();
  client_destruct(&client);
  reactor_destruct();

  net_ssl_free(ssl_ctx);
}
