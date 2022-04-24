#include "mmu.h"
#include "page_table.h"
#include "multiboot.h"
#include "printk.h"
#include <stddef.h>
#include <stdbool.h>
#include "string.h"

// Physical memory map information
memory_map_t mmap;

// Returns true if the address is within the range
static inline bool range_contains_addr(uint64_t addr, uint64_t start, uint64_t end) {
    return ((addr - start) < (end - start));
}

void push_free_pool_entry(struct free_pool_node *entry) {
    if (mmap.free_pool == NULL) {
        mmap.free_pool = entry;
    } else {
        entry->next = mmap.free_pool;
        mmap.free_pool = entry;
    }
}

struct free_pool_node *pop_free_pool_entry() {
    struct free_pool_node *res;
    if (mmap.free_pool == NULL) return NULL;
    res = mmap.free_pool;
    mmap.free_pool = mmap.free_pool->next;
    return res;
}

// Returns an address to a free page frame
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
        if (!range_contains_addr(mmap.current_page, mmap.current_region->start, mmap.current_region->end)) {
            // Page is outside of current memory region
            if (mmap.current_region->next == NULL) {
                printk("Tried to allocate address: %p\n", (void *)mmap.current_page);
                printk("ERROR: NO PHYSICAL MEMORY REMAINING!\n");
                while (1) asm("hlt");
            }
            // Set current free entry to next
            mmap.current_region = mmap.current_region->next;
            mmap.current_page = align_page(mmap.current_region->start);
        } 
        
        if (range_contains_addr(mmap.current_page, mmap.kernel.start, mmap.kernel.end)) {
            // Page is inside of kernel
            mmap.current_page = align_page(mmap.kernel.end);
            continue; // Recheck address
        }
        
        if (range_contains_addr(mmap.current_page, mmap.multiboot.start, mmap.multiboot.end)) {
            mmap.current_page = align_page(mmap.multiboot.end);
            continue; // Recheck address
        }

        page_valid = true;
    }

    page = mmap.current_page;
    mmap.current_page += PAGE_SIZE;
    return page;
}

// Frees the page frame associated with the address pf
void MMU_pf_free(physical_addr_t pf) {
    pf &= ~(PAGE_SIZE - 1);
    struct free_pool_node *entry;

    // Write a free pool node at the beginning of the page
    entry = (struct free_pool_node *)pf;
    entry->next = NULL;

    // Add free pool entry to the linked list
    push_free_pool_entry(entry);
}