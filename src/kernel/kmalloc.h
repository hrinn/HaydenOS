#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

// Kernel heap allocator
void kfree(void *addr);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);

#endif