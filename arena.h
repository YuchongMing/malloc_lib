#ifndef ARENA_H
#define ARENA_H

#include "buddy.h"
#include <pthread.h>
#include <sys/sysinfo.h>
#include <pthread.h>

typedef struct arena
{	
	int locked;

	pthread_mutex_t mutex;
	f_header* free_list[MAX_LEVEL];
	size_t alloc_request;
	size_t free_request;
	size_t total_size;
	size_t used_size;
	size_t free_size;
	size_t max_free_size;
} arena;

typedef struct mallinfo
{
	size_t sbrk_size;
	size_t mmap_size;
	int arena_num;
} mallinfo;

#define MAX_ARENA get_nprocs()

arena *arena_pool[129];
pthread_mutex_t loop_mutex;
mallinfo minfo;
extern __thread int th_arena_id;
extern __thread f_header **free_list;

arena *find_arena();
arena *create_arena(int i);
int extend_arena(arena *a);
void lock_arena(arena *a);
void unlock_arena(arena *a);
size_t check_size();

#endif