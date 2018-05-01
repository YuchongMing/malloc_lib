#ifndef MEMALIGN_H
#define MEMALIGN_H

#include <unistd.h>

void* memalign(size_t alignment, size_t size);

#endif