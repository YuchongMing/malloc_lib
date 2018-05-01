#include "realloc.h"
#include "ma_util.h"
#include "reallocarray.h"
#include <unistd.h>

void*
reallocarray(void* ptr, size_t nmemb, size_t size)
{
  size_t total = align8(nmemb * size);
  void* re = realloc(ptr, total);
  if (!re) {
    return NULL;
  }
  return re;
}