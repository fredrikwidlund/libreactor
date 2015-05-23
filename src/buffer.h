#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

typedef struct buffer buffer;

struct buffer
{
  char   *data;
  size_t  size;
  size_t  capacity;
};

/* allocators */

buffer *buffer_new();
void    buffer_free(buffer *);
void    buffer_init(buffer *);
char   *buffer_deconstruct(buffer *);

/* capacity */

size_t  buffer_size(buffer *);
size_t  buffer_capacity(buffer *);
int     buffer_reserve(buffer *, size_t);
int     buffer_compact(buffer *);

/* modifiers */

int     buffer_insert(buffer *, size_t, char *, size_t);
void    buffer_erase(buffer *, size_t, size_t);
void    buffer_clear(buffer *);

/* element access */

char   *buffer_data(buffer *);

/* internals */

size_t  buffer_roundup(size_t);

#endif /* BUFFER_H_INCLUDED */
