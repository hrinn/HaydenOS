#ifndef MEMDEF_H
#define MEMDEF_H

#include <stdint-gcc.h>

#pragma GCC diagnostic ignored "-Wpointer-arith"

#define KERNEL_TEXT_START 0xffff800000000000
#define VSPACE(func) (func + KERNEL_TEXT_START)

typedef uint64_t virtual_addr_t;
typedef uint64_t physical_addr_t;

#endif