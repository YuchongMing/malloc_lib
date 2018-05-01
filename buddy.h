#ifndef BUDDY_H
#define BUDDY_H

#include "buddy_util.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/sysinfo.h>

#define MAX_LEVEL 20

typedef struct f_header
{
  struct f_header* prev;
  struct f_header* next;
  void* self;
} f_header;

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

typedef struct pthread {
	pthread_t ptid;
	int a_id;
	struct pthread* next;
} pthread;

typedef struct mallinfo
{
	size_t sbrk_size;
	size_t mmap_size;
	int arena_num;
} mallinfo;

#define BASE next_pow2(sizeof(f_header))
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define MAX_ARENA get_nprocs()
#define LIMIT_SIZE PAGE_SIZE / 2
#define PAGE_LEVEL get_level(PAGE_SIZE / BASE)

arena *arena_pool[128];
pthread_mutex_t loop_mutex;
pthread_mutex_t store_mutex;
pthread *p_root;
mallinfo minfo;
extern __thread int th_arena_id;
extern __thread f_header **free_list;

arena *find_arena();
arena *create_arena(int i);
void store_pthread(int i);
int extend_arena(arena *a);
void lock_arena(arena *a);
void unlock_arena(arena *a);
size_t check_size();
void malloc_stats();
void* find_block(size_t size);
void split_block(f_header* fh, int top, int bottom);
void* combine_block(f_header* fh, int level, int max);
void* free_block(void* ptr, size_t size);
void* resize_block(void* ptr, size_t curt_size, size_t next_size);
void* poll_block(int level);
void insert_block(f_header* fh, int level);
void isolate_block(f_header* fh, int level);
f_header* search_buddy(void* fh, int level);
f_header* fusion_buddy(f_header* fh, f_header* buddy, int level);
f_header* set_f_header(void* ptr);

#endif