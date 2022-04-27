#ifndef MMU_H
#define MMU_H

#include "memdef.h"

#define PAGE_SIZE 4096

// Beginning of multiboot tags structure
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

// Memory management interface
physical_addr_t MMU_pf_alloc(void);
void MMU_pf_free(physical_addr_t pf);
void *MMU_alloc_page();
void *MMU_alloc_pages(int num);
void MMU_free_page(void *);
void MMU_free_pages(void *, int num);

// Initialization functions
void parse_multiboot_tags(struct multiboot_info *);
void setup_pml4(virtual_addr_t *);
void cleanup_old_virtual_space();

#endif