#ifndef REACTOR_POINTER_H_INCLUDED
#define REACTOR_POINTER_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

typedef void *pointer;

void pointer_move(pointer *, size_t);
void pointer_push(pointer *, data);
void pointer_push_byte(pointer *, uint8_t);

#endif /* REACTOR_POINTER_H_INCLUDED */
