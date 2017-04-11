#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <sys/queue.h>

#include <dynamic.h>

#include "reactor_memory.h"
#include "reactor_util.h"
#include "reactor_user.h"
#include "reactor_core.h"
#include "reactor_stream.h"
#include "reactor_http_parser.h"
#include "reactor_http.h"

#include "picohttpparser/picohttpparser.h"

void reactor_http_parser_construct(reactor_http_parser *parser)
{
  *parser = (reactor_http_parser) {0};
}

int reactor_http_parser_request(reactor_http_parser *parser, reactor_http_request *request,
                                     reactor_stream_data *data)
{
  size_t i;
  int header_size, body_size;

  if (reactor_unlikely(parser->complete_size && reactor_stream_data_size(data) < parser->complete_size))
    return 0;

  header_size = phr_parse_request(reactor_stream_data_base(data), reactor_stream_data_size(data),
                                  &request->method.base, &request->method.size,
                                  &request->path.base, &request->path.size,
                                  &request->version,
                                  (struct phr_header *) request->headers, &request->header_count, 0);
  if (reactor_unlikely(header_size == -1))
    return -1;

  if (reactor_unlikely(header_size == -2))
    return 0;

  body_size = 0;
  if (reactor_likely(!parser->complete_size))
    {
      for (i = 0; i < request->header_count; i ++)
        if (reactor_memory_equal_case(request->headers[i].name, reactor_memory_str("content-length")))
          body_size = strtoul(reactor_memory_base(request->headers[i].value), NULL, 10);

      parser->complete_size = header_size + body_size;
      if (reactor_unlikely(reactor_stream_data_size(data) < parser->complete_size))
        return 0;
    }

  request->body = reactor_memory_ref((char *) reactor_stream_data_base(data) + header_size, body_size);
  reactor_stream_data_consume(data, parser->complete_size);
  parser->complete_size = 0;
  return 1;
}

int reactor_http_parser_response(reactor_http_parser *parser, reactor_http_response *response,
                                      reactor_stream_data *data)
{
  size_t i;
  int header_size, body_size;

  if (reactor_unlikely(parser->complete_size && reactor_stream_data_size(data) < parser->complete_size))
    return 0;

  header_size = phr_parse_response(reactor_stream_data_base(data), reactor_stream_data_size(data),
                                   &response->version,
                                   &response->status,
                                   (const char **) &response->reason.base, &response->reason.size,
                                   (struct phr_header *) response->headers, &response->header_count, 0);
  if (reactor_unlikely(header_size == -1))
    return -1;

  if (reactor_unlikely(header_size == -2))
    return 0;

  body_size = -1;
  if (reactor_likely(!parser->complete_size))
    {
      for (i = 0; i < response->header_count; i ++)
        if (reactor_memory_equal_case(response->headers[i].name, reactor_memory_str("content-length")))
          body_size = strtoul(reactor_memory_base(response->headers[i].value), NULL, 10);
      if (reactor_unlikely(body_size == -1))
        return -1;

      parser->complete_size = header_size + body_size;
      if (reactor_unlikely(reactor_stream_data_size(data) < parser->complete_size))
        return 0;
    }

  response->body = reactor_memory_ref((char *) reactor_stream_data_base(data) + header_size, body_size);
  reactor_stream_data_consume(data, parser->complete_size);
  parser->complete_size = 0;
  return 1;
}
