#include "arena.h"
#include "buddy.h"
#include "ma_util.h"
#include "buddy_util.h"
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

__thread int th_arena_id = -1;
__thread f_header **free_list = NULL;

arena *
find_arena()
{	
	if (th_arena_id != -1) return arena_pool[th_arena_id];
	pthread_mutex_lock(&loop_mutex);
	int i;	
	for (i = 0; i < MAX_ARENA; i++) {
		if (arena_pool[i] != NULL) {
			continue;
		}
		create_arena(i);
		th_arena_id = i;
		pthread_mutex_unlock(&loop_mutex);
		free_list = arena_pool[i]->free_list;
		return arena_pool[i];
	}
	pthread_mutex_unlock(&loop_mutex);
	i = 0;
  while (arena_pool[i]->locked == 0) {
    i = (i + 1) % MAX_ARENA;
  }
  th_arena_id = i;
	return arena_pool[i];
}

arena *
create_arena(int i)
{	
	arena *a = mmap(0, sizeof(arena), 
			PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
	a->locked = 1;
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

  snprintf(buf, 1024, "Total arena number: %d\n", minfo.arena_num);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, 1024, "Total sbrk area size: %lu\n", minfo.sbrk_size);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, 1024, "Total mmap area size: %lu\n", minfo.mmap_size);
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  snprintf(buf, 1024, "For each arena:\n");
  write(STDOUT_FILENO, buf, strlen(buf) + 1);

  int i;
  for (i = 0; i < minfo.arena_num; i++) {
  	arena *a = arena_pool[i];
  	snprintf(buf, 1024, "Arena %d: total block size is %lu, used block size is %lu, free block size is %lu\n",
  		 i, a->total_size, a->used_size, a->free_size);
  	write(STDOUT_FILENO, buf, strlen(buf) + 1);

  	snprintf(buf, 1024, "Number of allocate request is %lu, number of free request is %lu\n",
  		 a->alloc_request, a->free_request);
  	write(STDOUT_FILENO, buf, strlen(buf) + 1);
  }
}