#ifndef STREAM_H_INCLUDED
#define STREAM_H_INCLUDED

#define STREAM_BLOCK_SIZE 65536

enum
{
  STREAM_OPEN   = 0x01,
  STREAM_SOCKET = 0x02
};

enum
{
  STREAM_ERROR,
  STREAM_READ,
  STREAM_FLUSH,
  STREAM_CLOSE
};

typedef struct stream stream;
struct stream
{
  core_handler user;
  int          flags;
  int          fd;
  int          events;
  int          next;
  buffer       input;
  buffer       output;
};

void    stream_construct(stream *, core_callback *, void *);
void    stream_destruct(stream *);
void    stream_open(stream *, int);
void    stream_close(stream *);
segment stream_read(stream *);
segment stream_read_line(stream *);
void    stream_write(stream *, segment);
segment stream_allocate(stream *, size_t);
void    stream_consume(stream *, size_t);
void    stream_notify(stream *);
void    stream_flush(stream *);
int     stream_is_open(stream *);
int     stream_is_socket(stream *);

#endif /* STREAM_H_INCLUDED */
