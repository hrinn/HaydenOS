#ifndef MMU_H
#define MMU_H

#include <stdint-gcc.h>

#define PAGE_SIZE 4096

typedef uint64_t virtual_addr_t;
typedef uint64_t physical_addr_t;

physical_addr_t MMU_pf_alloc(void);
void MMU_pf_free(physical_addr_t pf);
int get_allocated_pages(void);

#endif