#include "buddy.h"
#include "buddy_util.h"
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

__thread int th_arena_id = -1;
__thread f_header **free_list = NULL;

arena *
find_arena()
{ 
  if (th_arena_id != -1) {
    return arena_pool[th_arena_id];
  }

  pthread_t ptid = pthread_self();
  pthread *ptr = p_root;
  while (ptr != NULL) {
    if (ptid == ptr->ptid) {
      th_arena_id = ptr->a_id;
      free_list = arena_pool[ptr->a_id]->free_list;
      return arena_pool[ptr->a_id];
    }
    ptr = ptr->next;
  }
  pthread_mutex_lock(&loop_mutex);
  int i;  
  for (i = 0; i < MAX_ARENA; i++) {
    if (arena_pool[i] != NULL) {
      continue;
    }
    create_arena(i);
    th_arena_id = i;
    free_list = arena_pool[i]->free_list;
    store_pthread(i);
    pthread_mutex_unlock(&loop_mutex);
    return arena_pool[i];
  }
  pthread_mutex_unlock(&loop_mutex);
  i = 0;
  while (arena_pool[i]->locked == 0) {
    i = (i + 1) % MAX_ARENA;
  }
  th_arena_id = i;
  free_list = arena_pool[i]->free_list;
  pthread_mutex_lock(&store_mutex);
  store_pthread(i);
  pthread_mutex_unlock(&store_mutex);
  return arena_pool[i];
}

void
store_pthread(int i)
{
  pthread *p = mmap(0, sizeof(pthread), 
    PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  p->ptid = pthread_self();
  p->a_id = i;
  p->next = p_root;
  p_root = p;
}

arena *
create_arena(int i)
{ 
  arena *a = mmap(0, sizeof(arena), 
      PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  a->locked = 1;
  a->used_size = 0;
  a->free_size = 0;
  a->total_size = 0;
  a->max_free_size = LIMIT_SIZE;
  a->free_size += PAGE_SIZE;
  a->total_size += PAGE_SIZE;
  void *ptr = mmap(0, PAGE_SIZE, 
      PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  a->free_list[PAGE_LEVEL] = set_f_header(ptr);
  minfo.mmap_size += PAGE_SIZE;
  minfo.arena_num += 1;
  arena_pool[i] = a;
  return a;
}

int 
extend_arena(arena *a)
{
  void *ptr = mmap(0, PAGE_SIZE, 
      PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (ptr == (void *)-1) {
    return -1;
  }
  minfo.mmap_size += PAGE_SIZE;
  a->free_size += PAGE_SIZE;
  a->total_size += PAGE_SIZE;
  a->free_list[PAGE_LEVEL] = set_f_header(ptr);
  return 1;
}

size_t
check_size()
{
  int i = PAGE_LEVEL;
  while (i >= 0 && free_list[i] == NULL) {
    i--;
  }
  if (i == -1) {
    return 0;
  }
  return pow2(i) * BASE;
}

void
lock_arena(arena *a)
{
  pthread_mutex_lock(&a->mutex);
  a->locked = 0;
}

void
unlock_arena(arena *a)
{
  a->locked = 1;
  pthread_mutex_unlock(&a->mutex);
}

void 
malloc_stats()
{
  char buf[1024];

  snprintf(buf, 1024, "\n%-16s = %d\n","Arena number", minfo.arena_num);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, 1024, "%-16s = %lu\n","Sbrk size", minfo.sbrk_size);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, 1024, "%-16s = %lu\n","Mmap size", minfo.mmap_size);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  int i;
  for (i = 0; i < minfo.arena_num; i++) {
    arena *a = arena_pool[i];
    snprintf(buf, 1024, "\nArena %d:\n",i);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
    snprintf(buf, 1024, "%-16s = %lu\n", "Total size", a->total_size);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
    snprintf(buf, 1024, "%-16s = %lu\n", "Used size", a->used_size);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
    snprintf(buf, 1024, "%-16s = %lu\n", "Free size", a->free_size);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
    snprintf(buf, 1024, "%-16s = %lu\n", "Allocate request", a->alloc_request);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
    snprintf(buf, 1024, "%-16s = %lu\n", "Free request", a->free_request);
    write(STDOUT_FILENO, buf, strlen(buf) + 1);
  }
}

void*
find_block(size_t size)
{
  int level = get_level(next_pow2(size) / BASE);
  if (free_list[level]) {
    return poll_block(level);
  }
  int i;
  for (i = level + 1; i <= PAGE_LEVEL; i++) {
    if (free_list[i] == NULL) {
      continue;
    }
    split_block(free_list[i], i, level);
    return poll_block(level);
  }
  return NULL;
}

void
split_block(f_header* fh, int top, int bottom)
{
  if (top <= bottom) {
    return;
  }
  isolate_block(fh, top);
  int i;
  for (i = top; i > bottom; i--) {
    f_header* buddy = set_f_header((void*)fh + pow2(i - 1) * BASE);
    insert_block(buddy, i - 1);
  }
  insert_block(fh, bottom);
}

void*
combine_block(f_header* fh, int level, int max)
{
  f_header* buddy = search_buddy((void*)fh, level);
  while ((max == 0 || level < max) && level < PAGE_LEVEL && buddy &&
         buddy->self == (void*)buddy) {
    fh = fusion_buddy(fh, buddy, level);
    buddy = search_buddy((void*)fh, ++level);
  }
  return (void*)fh;
}

void*
free_block(void* ptr, size_t size)
{
  if (!ptr) {
    return (void*)-1;
  }
  f_header* fh = set_f_header(ptr);
  int level = get_level(next_pow2(size) / BASE);
  insert_block(fh, level);
  combine_block(fh, level, 0);
  return (void*)1;
}

void*
resize_block(void* ptr, size_t curt_size, size_t next_size)
{

  int curt_level = get_level(next_pow2(curt_size) / BASE);
  int next_level = get_level(next_pow2(next_size) / BASE);
  if (next_level > curt_level) {
    int l;
    for (l = curt_level; l < next_level; l++) {
      f_header* buddy = search_buddy(ptr, l);
      if ((void*)buddy == NULL || buddy->self != buddy) {
        return NULL;
      }
    }
  }
  uint32_t backup = *(uint32_t*)(ptr + 16);
  f_header* fh = set_f_header(ptr);
  if (next_level <= curt_level) {
    split_block(fh, curt_level, next_level);
  } else {
    f_header* buddy = combine_block(fh, curt_level, next_level);
    if ((void*)buddy != ptr) {
      memcpy((void*)buddy, ptr, curt_size);
      isolate_block(buddy, next_level);
      return (void*)buddy;
    }
  }
  *(uint32_t*)(ptr + 16) = backup;
  isolate_block(fh, next_level);
  return ptr;
}

void*
poll_block(int level)
{
  if (!free_list[level]) {
    return NULL;
  }
  f_header* fh = free_list[level];
  if (fh->next) {
    fh->next->prev = NULL;
  }
  free_list[level] = fh->next;
  fh->next = NULL;
  fh->self = NULL;
  return (void*)fh;
}

void
insert_block(f_header* fh, int level)
{
  if (free_list[level]) {
    f_header* root = free_list[level];
    fh->next = root;
    root->prev = fh;
  }
  free_list[level] = fh;
}

void
isolate_block(f_header* fh, int level)
{
  if (free_list[level] == fh) {
    free_list[level] = fh->next;
  }
  if (fh->prev)
    fh->prev->next = fh->next;
  if (fh->next)
    fh->next->prev = fh->prev;
  fh->prev = NULL;
  fh->next = NULL;
}

f_header*
search_buddy(void* fh, int level)
{
  int mask = 1 << (level + get_level(BASE));
  f_header* buddy = (f_header*)((uintptr_t)fh ^ mask);
  f_header* iter = free_list[level];
  while (iter != NULL) {
    if ((void*)iter == (void*)buddy) {
      return buddy;
    }
    if (!iter->next) {
      iter = NULL;
    } else {
      iter = iter->next;
    }
  }
  return (f_header*)0;
}

f_header*
fusion_buddy(f_header* fh, f_header* buddy, int level)
{
  f_header* stay = NULL;
  f_header* dump = NULL;
  if (((void*)fh - (void*)buddy) < 0) {
    stay = fh;
    dump = buddy;
  } else {
    stay = buddy;
    dump = fh;
  }
  isolate_block(dump, level);
  dump->self = NULL;
  isolate_block(stay, level);
  insert_block(stay, level + 1);
  return stay;
}

f_header*
set_f_header(void* ptr)
{
  f_header* fh = ptr;
  fh->next = NULL;
  fh->prev = NULL;
  fh->self = ptr;
  return fh;
}
