#include "posix_memalign.h"
#include "ma_util.h"
#include "malloc.h"
#include "memalign.h"
#include <unistd.h>

int
posix_memalign(void** memptr, size_t alignment, size_t size)
{
  void* ret = malloc(align(alignment, size));
  if (!ret) {
    return -1;
  }
  memptr = &ret;
  return 0;
}