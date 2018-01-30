#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#ifndef REACTOR_STREAM_BLOCK_SIZE
#define REACTOR_STREAM_BLOCK_SIZE 131072
#endif /* REACTOR_STREAM_BLOCK_SIZE */

enum reactor_stream_event
{
  REACTOR_STREAM_EVENT_ERROR,
  REACTOR_STREAM_EVENT_READ,
  REACTOR_STREAM_EVENT_WRITE,
  REACTOR_STREAM_EVENT_CLOSE
};

typedef struct reactor_stream reactor_stream;
struct reactor_stream
{
  reactor_user        user;
  reactor_descriptor  descriptor;
  buffer              read;
  buffer              write;
  int                 blocked;
};

int     reactor_stream_open(reactor_stream *, reactor_user_callback *, void *, int);
void    reactor_stream_close(reactor_stream *);
int     reactor_stream_blocked(reactor_stream *);
void   *reactor_stream_data(reactor_stream *);
size_t  reactor_stream_size(reactor_stream *);
void    reactor_stream_consume(reactor_stream *, size_t);
void    reactor_stream_write(reactor_stream *, void *, size_t);
void    reactor_stream_write_string(reactor_stream *, char *);
int     reactor_stream_flush(reactor_stream *);
buffer *reactor_stream_buffer(reactor_stream *);

#endif /* REACTOR_STREAM_H_INCLUDED */
