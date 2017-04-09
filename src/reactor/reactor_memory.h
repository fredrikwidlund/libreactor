#ifndef REACTOR_MEMORY_H_INCLUDED
#define REACTOR_MEMORY_H_INCLUDED

typedef struct reactor_memory reactor_memory;
struct reactor_memory
{
  const char *base;
  size_t      size;
};

reactor_memory  reactor_memory_ref(const char *, size_t);
reactor_memory  reactor_memory_str(const char *);
const char     *reactor_memory_base(reactor_memory);
size_t          reactor_memory_size(reactor_memory);
int             reactor_memory_empty(reactor_memory);
int             reactor_memory_equal(reactor_memory, reactor_memory);
int             reactor_memory_equal_case(reactor_memory, reactor_memory);

#endif /* REACTOR_MEMORY_H_INCLUDED */

