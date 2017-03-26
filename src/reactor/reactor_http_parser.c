#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <sys/queue.h>

#include <dynamic.h>
#include <reactor.h>

#include "picohttpparser/picohttpparser.h"

void reactor_http_parser_construct(reactor_http_parser *parser)
{
  *parser = (reactor_http_parser) {0};
}

int reactor_http_parser_read_request(reactor_http_parser *parser, reactor_http_request *request,
                                     reactor_stream_data *data)
{
  struct phr_header phr_header[request->header_count];
  size_t method_size, path_size, i;
  ssize_t body_size;
  int header_size;

  if (parser->complete_size && reactor_stream_data_size(data) < parser->complete_size)
    return 0;

  header_size = phr_parse_request(reactor_stream_data_base(data), reactor_stream_data_size(data),
                                  (const char **) &request->method, &method_size,
                                  (const char **) &request->path, &path_size,
                                  &request->version,
                                  phr_header, &request->header_count, 0);
  if (header_size == -1)
    return -1;

  if (header_size == -2)
    return 0;

  body_size = 0;
  if (!parser->complete_size)
    {
      for (i = 0; i < request->header_count; i ++)
        if (phr_header[i].name_len == strlen("content-length") &&
            strncasecmp(phr_header[i].name, "content-length", phr_header[i].name_len) == 0)
          body_size = strtoul(phr_header[i].value, NULL, 10);

      parser->complete_size = header_size + body_size;
      if (reactor_stream_data_size(data) < parser->complete_size)
        return 0;
    }

  request->method[method_size] = 0;
  request->path[path_size] = 0;
  request->data = (char *) reactor_stream_data_base(data) + header_size;
  request->size = body_size;
  reactor_stream_data_consume(data, parser->complete_size);
  parser->complete_size = 0;
  return 1;
}

int reactor_http_parser_read_response(reactor_http_parser *parser, reactor_http_response *response,
                                      reactor_stream_data *data)
{
  struct phr_header phr_header[response->header_count];
  size_t reason_size, i;
  ssize_t body_size;
  int header_size;

  if (parser->complete_size && reactor_stream_data_size(data) < parser->complete_size)
    return 0;

  header_size = phr_parse_response(reactor_stream_data_base(data), reactor_stream_data_size(data),
                         &response->version,
                         &response->status,
                         (const char **) &response->reason, &reason_size,
                         phr_header, &response->header_count, 0);
  if (header_size == -1)
    return -1;

  if (header_size == -2)
    return 0;

  body_size = -1;
  if (!parser->complete_size)
    {
      for (i = 0; i < response->header_count; i ++)
        if (phr_header[i].name_len == strlen("content-length") &&
            strncasecmp(phr_header[i].name, "content-length", phr_header[i].name_len) == 0)
          body_size = strtoul(phr_header[i].value, NULL, 10);
      if (body_size == -1)
        return -1;

      parser->complete_size = header_size + body_size;
      if (reactor_stream_data_size(data) < parser->complete_size)
        return 0;
    }

  response->data = (char *) reactor_stream_data_base(data) + header_size;
  response->size = body_size;
  reactor_stream_data_consume(data, parser->complete_size);
  parser->complete_size = 0;
  return 1;
}

