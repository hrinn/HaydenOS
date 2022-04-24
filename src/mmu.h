#ifndef MMU_H
#define MMU_H

#include "memdef.h"

physical_addr_t MMU_pf_alloc(void);
void MMU_pf_free(physical_addr_t pf);

struct free_pool_node {
    struct free_pool_node *next;
};

struct free_mem_region {
    physical_addr_t start;
    physical_addr_t end;
    struct free_mem_region *next;
};

struct addr_range {
    physical_addr_t start;
    physical_addr_t end;
};

typedef struct mmap {
    struct addr_range kernel;
    struct addr_range multiboot;
    struct free_mem_region *first_region;
    struct free_mem_region *current_region;
    physical_addr_t current_page;
    struct free_pool_node *free_pool;
    struct multiboot_elf_tag *elf_tag;
} memory_map_t;

static inline uint64_t align_page(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

#endif