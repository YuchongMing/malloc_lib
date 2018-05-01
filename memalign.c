#include "memalign.h"
#include "ma_util.h"
#include "malloc.h"
#include <unistd.h>

void*
memalign(size_t alignment, size_t size)
{
  return malloc(align(alignment, size));
}