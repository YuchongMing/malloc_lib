#include "ma_util.h"
#include "buddy_util.h"
#include <string.h>

m_header*
set_m_header(void* ptr, size_t size)
{
  m_header* mh = ptr;
  mh->size = size;
  mh->self = ptr;
  return mh;
}

size_t
align(size_t alignment, size_t size)
{
  int offset = get_level(alignment);
  return (((((size)-1) >> offset) << offset) + alignment);
}

size_t
align8(size_t size)
{
  return (((((size)-1) >> 3) << 3) + 8);
}