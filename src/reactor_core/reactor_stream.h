#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#ifndef REACTOR_STREAM_BUFFER_SIZE
#define REACTOR_STREAM_BUFFER_SIZE 1048576
#endif

enum REACTOR_STREAM_EVENTS
{
  REACTOR_STREAM_ERROR           = 0x01,
  REACTOR_STREAM_DATA            = 0x02,
  REACTOR_STREAM_WRITE_BLOCKED   = 0x04,
  REACTOR_STREAM_WRITE_AVAILABLE = 0x08,
  REACTOR_STREAM_END             = 0x10,
  REACTOR_STREAM_CLOSE           = 0x20
};

enum REACTOR_STREAM_STATE
{
  REACTOR_STREAM_CLOSED = 0,
  REACTOR_STREAM_OPEN,
  REACTOR_STREAM_LINGER,
  REACTOR_STREAM_CLOSING
};

typedef struct reactor_stream reactor_stream;
struct reactor_stream
{
  int           state;
  reactor_user  user;
  reactor_desc  desc;
  int           flags;
  buffer        input;
  buffer        output;
  size_t        written;
};

typedef struct reactor_stream_data reactor_stream_data;
struct reactor_stream_data
{
  char         *base;
  size_t        size;
};

void reactor_stream_init(reactor_stream *, reactor_user_call *, void *);
void reactor_stream_user(reactor_stream *, reactor_user_call *, void *);
int  reactor_stream_open(reactor_stream *, int);
void reactor_stream_error(reactor_stream *);
void reactor_stream_event(void *, int, void *);
void reactor_stream_read(reactor_stream *);
int  reactor_stream_write(reactor_stream *, char *, size_t);
int  reactor_stream_puts(reactor_stream *, char *);
int  reactor_stream_putu(reactor_stream *, uint32_t);
int  reactor_stream_printf(reactor_stream *, const char *, ...);
void reactor_stream_flush(reactor_stream *);
void reactor_stream_close(reactor_stream *);
void reactor_stream_data_init(reactor_stream_data *, char *, size_t);
void reactor_stream_data_consume(reactor_stream_data *, size_t);

#endif /* REACTOR_STREAM_H_INCLUDED */
