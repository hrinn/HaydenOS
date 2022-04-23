#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "mmu.h"

#define NUM_ENTRIES 512

// Entry flag positions
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER_ACCESS 0x4
#define PAGE_NO_EXECUTE 0x8000000000000000

// PML4 entries
#define PML4_IDENTITY_MAP 0
#define PAGE_OFFSET 12
#define VGA_ADDR 0xB8000

// Offsets in Virtual Address Space
#define KERNEL_TEXT_START 0xFFFF800000000000
#define KERNEL_STACKS_START 0xFFFFFF0000000000

#define STACK_SIZE 4096 * 2

typedef struct page_table_entry {
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user : 1;
    uint64_t write_through : 1;
    uint64_t disable_cache : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t huge : 1;
    uint64_t global : 1;
    uint64_t avl1 : 3;
    uint64_t base_addr : 40;
    uint64_t avl2 : 11;
    uint64_t no_execute : 1;
} __attribute__((packed)) pt_entry_t;

typedef struct page_table {
    pt_entry_t table[NUM_ENTRIES];
} __attribute__((packed)) page_table_t;

physical_addr_t setup_pml4(void);

#endif