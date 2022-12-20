#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#include "memdef.h"

virtual_addr_t allocate_thread_stack();
void free_thread_stack(virtual_addr_t top);

#endif