#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "memdef.h"

#define PAGE_WRITABLE 0x2
#define PAGE_NO_EXECUTE 0x8000000000000000
#define PAGE_ALLOCATED 0x200
#define PAGE_HUGE 0x80
#define PML4_MMAP_INDEX 256

#define GET_VIRT_ADDR(PHYS_ADDR) (PHYS_ADDR + KERNEL_MMAP_START)
#define GET_PHYS_ADDR(VIRT_ADDR) (VIRT_ADDR - KERNEL_MMAP_START)

typedef struct permission {
    uint8_t w : 1;
    uint8_t x : 1;
    uint8_t res : 6;
} __attribute__((packed)) permission_t;

void map_page(virtual_addr_t virt_addr, physical_addr_t phys_addr, uint64_t flags);
int free_pf_from_virtual_addr(virtual_addr_t addr);
void setup_pml4();
void free_multiboot_sections();
void user_allocate_range(virtual_addr_t start, size_t size, permission_t perms);

#endif