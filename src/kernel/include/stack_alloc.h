#ifndef STACK_ALLOC_H
#define STACK_ALLOC_H

#include "memdef.h"

virtual_addr_t MMU_alloc_stack();
void MMU_free_stack(virtual_addr_t top);

#endif