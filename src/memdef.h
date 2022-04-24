#ifndef MEMDEF_H
#define MEMDEF_H

#include <stdint-gcc.h>

#define PAGE_SIZE 4096
#define NUM_ENTRIES 512
#define STACK_SIZE PAGE_SIZE * 2
#define KERNEL_TEXT_START 0xffff800000000000
#define KERNEL_STACKS_START 0xffffff8000000000

typedef uint64_t virtual_addr_t;
typedef uint64_t physical_addr_t;

#endif