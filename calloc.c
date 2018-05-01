#include "calloc.h"
#include "ma_util.h"
#include "malloc.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

void*
calloc(size_t nmemb, size_t size)
{
  size_t total = align8(nmemb * size);
  void* p = malloc(total);
  if (!p) {
    return p;
  }
  return memset(p, 0, total);
}