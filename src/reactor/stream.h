#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#include <openssl/ssl.h>

#include "data.h"
#include "buffer.h"
#include "reactor.h"
#include "descriptor.h"

#define STREAM_RECEIVE_SIZE 65536

enum stream_event_type
{
  STREAM_CLOSE = DESCRIPTOR_CLOSE,
  STREAM_READ  = DESCRIPTOR_READ,
  STREAM_WRITE = DESCRIPTOR_WRITE
};

typedef struct stream stream;
struct stream
{
  reactor_handler   handler;
  descriptor        descriptor;
  int               is_socket;
  int               mask;
  int               notify;
  size_t            input_block_size;
  buffer            input;
  size_t            output_block_size;
  buffer            output;
  SSL              *ssl;
  int               ssl_state;
};

void  stream_construct(stream *, reactor_callback *, void *);
void  stream_destruct(stream *);
void  stream_open(stream *, int, int, SSL_CTX *);
void  stream_notify(stream *);
void  stream_close(stream *);
void  stream_write_notify(stream *);
data  stream_read(stream *);
void  stream_consume(stream *, size_t);
void  stream_write(stream *, data);
void *stream_allocate(stream *, size_t);
void  stream_flush(stream *);

/* stream_receive_size(), stream_send_size() */
/* stream_close()? stream_allocate()? stream_read_line()? */

#endif /* REACTOR_STREAM_H_INCLUDED */
