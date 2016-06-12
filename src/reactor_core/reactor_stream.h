#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#ifndef REACTOR_STREAM_BLOCK_SIZE
#define REACTOR_STREAM_BLOCK_SIZE 65536
#endif

enum reactor_stream_state
{
  REACTOR_STREAM_CLOSED,
  REACTOR_STREAM_OPEN,
  REACTOR_STREAM_LINGER,
  REACTOR_STREAM_INVALID,
  REACTOR_STREAM_CLOSE_WAIT,
};

enum reactor_stream_events
{
  REACTOR_STREAM_ERROR,
  REACTOR_STREAM_READ,
  REACTOR_STREAM_WRITE_BLOCKED,
  REACTOR_STREAM_WRITE_AVAILABLE,
  REACTOR_STREAM_SHUTDOWN,
  REACTOR_STREAM_CLOSE
};

enum reactor_stream_flags
{
  REACTOR_STREAM_FLAGS_BLOCKED = 0x01
};

typedef struct reactor_stream reactor_stream;
struct reactor_stream
{
  int           state;
  int           flags;
  reactor_user  user;
  reactor_desc  desc;
  buffer        input;
  buffer        output;
  int           ref;
};

typedef struct reactor_stream_data reactor_stream_data;
struct reactor_stream_data
{
  char         *base;
  size_t        size;
};

void   reactor_stream_init(reactor_stream *, reactor_user_callback *, void *);
void   reactor_stream_open(reactor_stream *, int);
void   reactor_stream_close(reactor_stream *);
void   reactor_stream_shutdown(reactor_stream *);
void   reactor_stream_event(void *, int, void *);
void   reactor_stream_error(reactor_stream *);
void   reactor_stream_read(reactor_stream *);
void   reactor_stream_write(reactor_stream *, void *, size_t);
void   reactor_stream_write_direct(reactor_stream *, void *, size_t);
void   reactor_stream_write_string(reactor_stream *, char *);
void   reactor_stream_write_unsigned(reactor_stream *, uint32_t);
void   reactor_stream_flush(reactor_stream *);
void   reactor_stream_consume(reactor_stream_data *, size_t);

#endif /* REACTOR_STREAM_H_INCLUDED */
