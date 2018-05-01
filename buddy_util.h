#ifndef BUDDY_UTIL_H
#define BUDDY_UTIL_H

#include <unistd.h>

int get_level(int x);
size_t pow2(int i);
size_t next_pow2(size_t x);
int is_pow2(size_t x);

#endif