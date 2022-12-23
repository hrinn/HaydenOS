#include "pf_alloc.h"
#include "memdef.h"
#include "stddef.h"
#include "stdbool.h"
#include "multiboot.h"
#include "error.h"
#include "page_table.h"

struct option {
    bool present;
    physical_addr_t addr;
};

struct free_pool_node {
    struct free_pool_node *next;
};

typedef struct pf_info {
    struct free_pool_node *free_pool;
    physical_addr_t  current_page;
    uint8_t physical_region_index;
} pf_info_t;

static pf_info_t pf_info;
extern memory_map_t mmap;

static inline uint64_t align_page(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// Returns true if the address is within the range
static inline bool range_contains_addr(uint64_t addr, uint64_t start, uint64_t end) {
    return ((addr - start) < (end - start));
}

void push_free_pf(physical_addr_t pf) {
    // Convert physical address to writeable virtual one
    // Write a free pool node at the beginning of the page
    struct free_pool_node *node = (struct free_pool_node *)GET_VIRT_ADDR(pf);
    node->next = NULL;

    if (pf_info.free_pool == NULL) {
        pf_info.free_pool = node;
    } else {
        node->next = pf_info.free_pool;
        pf_info.free_pool = node;
    }
}

struct option pop_free_pf() {
    struct option res;

    if (pf_info.free_pool == NULL) {
        res.present = false;
        return res;
    }

    res.addr = GET_PHYS_ADDR((virtual_addr_t)pf_info.free_pool);
    res.present = true;

    pf_info.free_pool = pf_info.free_pool->next;

    return res;
}

void MMU_init_pf_alloc() {
    pf_info.current_page = align_page(mmap.physical_regions[0].start);
    pf_info.free_pool = NULL;
    pf_info.physical_region_index = 0;
}

// Allocates a physical page frame
physical_addr_t MMU_pf_alloc(void) {
    physical_addr_t page;
    struct option pf_option;
    bool page_valid;

    // Check the free pool linked list first
    pf_option = pop_free_pf();
    if (pf_option.present) {
        return pf_option.addr;
    }

    page_valid = false;
    while (!page_valid) {
        if (!range_contains_addr(pf_info.current_page, 
            mmap.physical_regions[pf_info.physical_region_index].start, 
            mmap.physical_regions[pf_info.physical_region_index].end))
        {
            // Page is outside of current memory region
            if (pf_info.physical_region_index >= mmap.num_regions) {
                blue_screen("MMU_pf_alloc(): No physical memory remaining!");
                while (1) asm("hlt");
            }
            // Set current free entry to next
            pf_info.physical_region_index++;
            pf_info.current_page = align_page(mmap.physical_regions[pf_info.physical_region_index].start);
        } 
        
        if (range_contains_addr(pf_info.current_page, mmap.kernel.start, mmap.kernel.end)) {
            // Page is inside of kernel
            pf_info.current_page = align_page(mmap.kernel.end);
            continue; // Recheck address
        }
        
        if (range_contains_addr(pf_info.current_page, mmap.multiboot.start, mmap.multiboot.end)) {
            pf_info.current_page = align_page(mmap.multiboot.end);
            continue; // Recheck address
        }

        page_valid = true;
    }

    page = pf_info.current_page;
    pf_info.current_page += PAGE_SIZE;
    return page;
}

// Returns a physical page frame to the free pool
void MMU_pf_free(physical_addr_t pf) {
    pf &= ~(PAGE_SIZE - 1);

    push_free_pf(pf);
}