#ifndef DYNAMIC_BUFFER_H_INCLUDED
#define DYNAMIC_BUFFER_H_INCLUDED

#include <stdint.h>

typedef struct buffer buffer;
struct buffer
{
  void   *data;
  size_t  size;
  size_t  capacity;
};

/* constructor/destructor */
void    buffer_construct(buffer *);
void    buffer_destruct(buffer *);

/* capacity */
size_t  buffer_size(buffer *);
size_t  buffer_capacity(buffer *);
void    buffer_reserve(buffer *, size_t);
void    buffer_resize(buffer *, size_t);
void    buffer_compact(buffer *);

/* modifiers */
void    buffer_insert(buffer *, size_t, void *, size_t);
void    buffer_insert_fill(buffer *, size_t, size_t, void *, size_t);
void    buffer_erase(buffer *, size_t, size_t);
void    buffer_clear(buffer *);

/* element access */
void   *buffer_data(buffer *);

#endif /* DYNAMIC_BUFFER_H_INCLUDED */
