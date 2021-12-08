#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#include <openssl/ssl.h>

#include "buffer.h"
#include "reactor.h"
#include "descriptor.h"

#define STREAM_RECEIVE_SIZE (2 * 65536)

enum stream_event_type
{
  STREAM_READ,
  STREAM_WRITE,
  STREAM_CLOSE
};

typedef struct stream stream;
struct stream
{
  reactor_handler  handler;
  descriptor       descriptor;
  buffer           input;
  buffer           output;
  int              write_notify;
  ssize_t          errors;
  SSL             *ssl;
  int              ssl_state;
};

void  stream_construct(stream *, reactor_callback *, void *);
void  stream_destruct(stream *);
void  stream_open(stream *, int, SSL_CTX *, int);
void  stream_close(stream *);
void  stream_write_notify(stream *);
void  stream_read(stream *, void **, size_t *);
void  stream_consume(stream *, size_t);
void  stream_write(stream *, void *, size_t);
void *stream_allocate(stream *, size_t);
void  stream_flush(stream *);

/* stream_receive_size(), stream_send_size() */
/* stream_close()? stream_allocate()? stream_read_line()? */

#endif /* REACTOR_STREAM_H_INCLUDED */
