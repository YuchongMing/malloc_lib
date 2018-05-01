#include "malloc.h"
#include "buddy.h"
#include "buddy_util.h"
#include "ma_util.h"
#include <stdint.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

void*
malloc(size_t size)
{ 

  if (size <= 0) {
    return NULL;
  }
  size = align8(size);
  size_t allocSize = size + HEADER_SIZE;

  if (allocSize > LIMIT_SIZE) {
    void* ret = mmap(0, allocSize, PROT_READ | PROT_WRITE,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (!ret) {
      return NULL;
    }
    m_header* mh = set_m_header(ret, size);
    return (void*)mh + HEADER_SIZE;
  }

  arena *a = find_arena();

  lock_arena(a);
  if (a->max_free_size < allocSize) {
    extend_arena(a);
  }
  void* ptr = find_block(allocSize);
  a->max_free_size = check_size();

  if (ptr == NULL) {
    unlock_arena(a);
    return NULL;
  }
  size_t block_size = next_pow2(allocSize);
  a->alloc_request += 1;
  a->used_size += block_size;
  a->free_size -= block_size;
  m_header* mh = set_m_header(ptr, size);
  unlock_arena(a);
  return (void*)mh + HEADER_SIZE;
}