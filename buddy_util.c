#include "buddy_util.h"
#include <stdlib.h>

int
get_level(int x)
{
  int r = 0;
  while (!(x & 1)) {
    x >>= 1;
    r++;
  }
  return r;
}

size_t
pow2(int i)
{
  if (i < 0) {
    return -1;
  }
  return 1 << i;
}

size_t
next_pow2(size_t x)
{
  if (is_pow2(x)) {
    return x;
  }
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return x + 1;
}

int
is_pow2(size_t x)
{
  return (x & (x - 1)) == 0 ? 1 : 0;
}