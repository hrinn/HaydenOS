#ifndef MEMDEF_H
#define MEMDEF_H

#include <stdint-gcc.h>

#define KERNEL_TEXT_START 0xffffffff80000000
#define USER_TEXT_START 0x8000000000
#define USER_STACK_START 0x7f8000000000

typedef uint64_t virtual_addr_t;
typedef uint64_t physical_addr_t;

#endif