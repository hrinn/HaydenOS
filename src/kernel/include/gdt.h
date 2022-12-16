#ifndef GDT_H
#define GDT_H

#include "memdef.h"

void GDT_remap(void);
void TSS_init(void);
void TSS_remap(virtual_addr_t *stack_tops, int n);
void TSS_set_ist(virtual_addr_t stack_top, int ist);
void TSS_set_rsp(virtual_addr_t stack_top, int rsp);

#define KERNEL_CODE_SELECTOR 0x8
#define KERNEL_DATA_SELECTOR 0x10
#define USER_CODE_SELECTOR 0x18
#define USER_DATA_SELECTOR 0x20

#define KERNEL_DPL 0
#define USER_DPL 3

#endif