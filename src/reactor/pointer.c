#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "data.h"
#include "utility.h"
#include "pointer.h"

void pointer_move(pointer *pointer, size_t size)
{
  *(uint8_t **) pointer += size;
}

void pointer_push(pointer *pointer, data data)
{
  memcpy(*pointer, data_base(data), data_size(data));
  pointer_move(pointer, data_size(data));
}

void pointer_push_byte(pointer *pointer, uint8_t byte)
{
  **((uint8_t **) pointer) = byte;
  pointer_move(pointer, 1);
}
