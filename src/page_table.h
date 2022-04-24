#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "memdef.h"

void setup_pml4(virtual_addr_t *);
void cleanup_old_virtual_space();

#endif