#ifndef GDT_H
#define GDT_H

#include "memdef.h"

void GDT_remap(void);
void TSS_init(void);
void TSS_remap(virtual_addr_t *stack_tops, int n);

// Offsets
#define KERNEL_CODE_OFFSET 0x8

#endif