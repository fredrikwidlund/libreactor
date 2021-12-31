#ifndef REACTOR_DATA_H_INCLUDED
#define REACTOR_DATA_H_INCLUDED

#include <stdint.h>

typedef struct data data;

struct data
{
  const void *base;
  size_t      size;
};

data        data_alloc(size_t);
void        data_free(data);
data        data_construct(const char *, size_t);
data        data_select(data, size_t, size_t);
data        data_null(void);
data        data_string(const char *);
const void *data_base(data);
size_t      data_size(data);
int         data_empty(data);
int         data_equal(data, data);
int         data_prefix(data, data);
size_t      data_offset(data, data);

#endif /* REACTOR_DATA_H_INCLUDED */
