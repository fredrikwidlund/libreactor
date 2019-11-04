#ifndef REACTOR_REACTOR_VECTOR_H_INCLUDED
#define REACTOR_REACTOR_VECTOR_H_INCLUDED

typedef struct reactor_vector reactor_vector;

struct reactor_vector
{
  void   *base;
  size_t  size;
};

reactor_vector reactor_vector_data(void *, size_t);
reactor_vector reactor_vector_string(char *);
reactor_vector reactor_vector_empty(void);
int            reactor_vector_equal(reactor_vector , reactor_vector);
int            reactor_vector_equal_case(reactor_vector, reactor_vector);
reactor_vector reactor_vector_copy(reactor_vector);
void           reactor_vector_clear(reactor_vector);

#endif /* REACTOR_REACTOR_VECTOR_H_INCLUDED */
