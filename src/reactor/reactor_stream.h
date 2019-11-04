#ifndef REACTOR_REACTOR_STREAM_H_INCLUDED
#define REACTOR_REACTOR_STREAM_H_INCLUDED

#define REACTOR_STREAM_BLOCK_SIZE 16384

enum reactor_stream_events
{
 REACTOR_STREAM_EVENT_ERROR,
 REACTOR_STREAM_EVENT_DATA,
 REACTOR_STREAM_EVENT_CLOSE
};

enum reactor_stream_flags
{
 REACTOR_STREAM_FLAG_WRITE = 0x01,
 REACTOR_STREAM_FLAG_SHUTDOWN = 0x02
};

typedef struct reactor_stream reactor_stream;
struct reactor_stream
{
  reactor_user  user;
  reactor_fd    fd;
  buffer        input;
  buffer        output;
  int           flags;
};

void            reactor_stream_construct(reactor_stream *, reactor_user_callback *, void *);
void            reactor_stream_user(reactor_stream *, reactor_user_callback *, void *);
void            reactor_stream_destruct(reactor_stream *);
void            reactor_stream_reset(reactor_stream *);
void            reactor_stream_open(reactor_stream *, int);
void           *reactor_stream_data(reactor_stream *);
size_t          reactor_stream_size(reactor_stream *);
void            reactor_stream_consume(reactor_stream *, size_t);
void           *reactor_stream_segment(reactor_stream *, size_t);
void            reactor_stream_write(reactor_stream *, void *, size_t);
reactor_status  reactor_stream_flush(reactor_stream *);
void            reactor_stream_shutdown(reactor_stream *);

#endif /* REACTOR_REACTOR_STREAM_H_INCLUDED */
