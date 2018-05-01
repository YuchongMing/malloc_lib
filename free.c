#include "free.h"
#include "buddy.h"
#include "buddy_util.h"
#include "ma_util.h"
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

void
free(void* ptr)
{ 

  if (!ptr) {
    return;
  }
  m_header* mh = ptr - HEADER_SIZE;
  if (mh->self != (void*)mh) {
    return;
  }
  size_t freeSize = mh->size + HEADER_SIZE;
  if (freeSize > LIMIT_SIZE) {
    munmap((void*)mh, freeSize);
    return;
  }
  arena *a = find_arena();
  lock_arena(a);
  free_block((void*)mh, freeSize);
  a->max_free_size = check_size();

  size_t block_size = next_pow2(freeSize);
  a->free_request += 1;
  a->used_size -= block_size;
  a->free_size += block_size;
  unlock_arena(a);
  return;
}