#ifndef MEMDEF_H
#define MEMDEF_H

#include <stdint-gcc.h>

#define KERNEL_TEXT_START       0xffffffff80000000  // Last 2GB
#define KERNEL_STACKS_START     0xffffff8000000000  // PML4[511]
#define KERNEL_HEAP_START       0xffff808000000000  // PML4[257]
#define KERNEL_IMAP_START       0xffff800000000000  // PML4[256]

#define USER_TEXT_START     0x8000000000
#define USER_STACK_START    0x7f8000000000

typedef uint64_t virtual_addr_t;
typedef uint64_t physical_addr_t;
typedef uint64_t size_t;

#define PAGE_SIZE 4096
#define STACK_SIZE PAGE_SIZE * 2

struct addr_range {
    uint64_t start;
    uint64_t end;
};

#endif