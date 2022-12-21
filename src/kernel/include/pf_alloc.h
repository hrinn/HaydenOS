#ifndef PF_ALLOC_H
#define PF_ALLOC_H

#include "memdef.h"

void MMU_init_pf_alloc();
physical_addr_t MMU_pf_alloc(void);
void MMU_pf_free(physical_addr_t pf);

#endif