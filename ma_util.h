#ifndef MA_UTIL_H
#define MA_UTIL_H

#include <unistd.h>

typedef struct m_header
{
  size_t size;
  void* self;
} m_header;

#define HEADER_SIZE sizeof(m_header)

m_header* set_m_header(void* ptr, size_t size);
size_t align(size_t alignment, size_t size);
size_t align8(size_t size);

#endif