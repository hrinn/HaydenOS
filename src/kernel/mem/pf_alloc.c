#include "pf_alloc.h"
#include "memdef.h"
#include "stddef.h"
#include "stdbool.h"
#include "multiboot.h"
#include "error.h"

// Returns true if the address is within the range
struct free_pool_node {
    struct free_pool_node *next;
};

typedef struct pf_info {
    struct free_pool_node *free_pool;
    physical_addr_t  current_page;
    struct free_mem_region *current_region;
} pf_info_t;

static pf_info_t pf_info;
extern memory_map_t mmap;

static inline uint64_t align_page(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

static inline bool range_contains_addr(uint64_t addr, uint64_t start, uint64_t end) {
    return ((addr - start) < (end - start));
}

void push_free_pool_entry(struct free_pool_node *entry) {
    if (pf_info.free_pool == NULL) {
        pf_info.free_pool = entry;
    } else {
        entry->next = pf_info.free_pool;
        pf_info.free_pool = entry;
    }
}

struct free_pool_node *pop_free_pool_entry() {
    struct free_pool_node *res;
    if (pf_info.free_pool == NULL) return NULL;
    res = pf_info.free_pool;
    pf_info.free_pool = pf_info.free_pool->next;
    return res;
}

void MMU_init_pf_alloc() {
    pf_info.current_page = align_page(mmap.first_region->start);
    pf_info.free_pool = NULL;
    pf_info.current_region = mmap.first_region;
}

// Allocates a physical page frame
physical_addr_t MMU_pf_alloc(void) {
    physical_addr_t page;
    struct free_pool_node *node;
    bool page_valid;

    // Check the free pool linked list first
    if ((node = pop_free_pool_entry()) != NULL) {
        return (physical_addr_t)node;
    }

    page_valid = false;
    while (!page_valid) {
        if (!range_contains_addr(pf_info.current_page, pf_info.current_region->start, pf_info.current_region->end)) {
            // Page is outside of current memory region
            if (pf_info.current_region->next == NULL) {
                blue_screen("MMU_pf_alloc: No physical memory remaining");
                while (1) asm("hlt");
            }
            // Set current free entry to next
            pf_info.current_region = pf_info.current_region->next;
            pf_info.current_page = align_page(pf_info.current_region->start);
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
    struct free_pool_node *entry;

    // Write a free pool node at the beginning of the page
    entry = (struct free_pool_node *)pf;
    entry->next = NULL;

    // Add free pool entry to the linked list
    push_free_pool_entry(entry);
}