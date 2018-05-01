#include "realloc.h"
#include "buddy.h"
#include "free.h"
#include "ma_util.h"
#include "malloc.h"
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

void*
realloc(void* ptr, size_t size)
{
  if (!ptr) {
    return malloc(size);
  }
  if (!size) {
    free(ptr);
    return NULL;
  }
  m_header* mh = ptr - HEADER_SIZE;

  if (mh->self != (void*)mh) {
    return (void*)-1;
  }
  size = align8(size);
  size_t curt_size = mh->size + HEADER_SIZE;
  size_t next_size = size + HEADER_SIZE;

  if (curt_size == next_size) {
    return ptr;
  }
  if (next_size > LIMIT_SIZE) {
    void* map_ptr = mmap(0, next_size, PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    m_header* next_mh = set_m_header(map_ptr, size);
    memcpy(map_ptr + HEADER_SIZE, ptr, mh->size);
    free(ptr);
    return (void*)next_mh + HEADER_SIZE;
  }

  if (next_size <= LIMIT_SIZE && curt_size > LIMIT_SIZE) {
    void* re = malloc(size);
    memcpy(re, ptr, size);
    munmap((void*)mh, curt_size);
    return re;
  }

  arena *a = find_arena();
  lock_arena(a);
  void* re = resize_block((void*)mh, curt_size, next_size);
  if (re) {
    size_t next_block_size = next_pow2(next_size);
    size_t curt_block_size = next_pow2(curt_size);
    a->max_free_size = check_size();
    a->used_size += next_block_size;
    a->used_size -= curt_block_size;
    a->free_size += curt_block_size;
    a->free_size -= next_block_size;
    unlock_arena(a);
    mh = set_m_header(re, size);
    return (void*)mh + HEADER_SIZE;
  }
  unlock_arena(a);
  re = malloc(size);
  if (!re) {
    return re;
  }
  memcpy(re, ptr, mh->size);
  free(ptr);
  return re;
}