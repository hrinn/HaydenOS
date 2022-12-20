#include <stddef.h>
#include "printk.h"
#include "string.h"
#include "error.h"
#include "page_table.h"

static virtual_addr_t kernel_brk = KERNEL_HEAP_START; // Next available page

void *MMU_alloc_page() {
    virtual_addr_t address;

    if (kernel_brk >= KERNEL_PSTACKS_START) {
        printk("MMU_alloc_page: exhausted terabytes of kernel heap space!\n");
        return NULL;
    }

    // Map a page at the current heap top
    // Leave this page as not present, but set allocated bit for on demand paging
    map_page(kernel_brk, 0, PAGE_ALLOCATED | PAGE_WRITABLE | PAGE_NO_EXECUTE);
    
    address = kernel_brk;
    kernel_brk += PAGE_SIZE;
    return (void *)address;
}

void *MMU_alloc_pages(int num) {
    int i;
    void *start_address = NULL, *temp;

    for (i = 0; i < num; i++) {
        temp = MMU_alloc_page();
        if (i == 0) start_address = temp;
    }

    return start_address;
}

void MMU_free_page(void *address) {
    virtual_addr_t vaddr = (virtual_addr_t)address;

    if (vaddr < KERNEL_HEAP_START || vaddr >= KERNEL_PSTACKS_START) {
        printk("MMU_free_page: tried to free an invalid address\n");
        return;
    }

    free_pf_from_virtual_addr(vaddr);

    // There is so much space available in the virtual memory
    // That we don't have to track freed pages
}

void MMU_free_pages(void *start_address, int num) {
    int i;

    for (i = 0; i < num; i++) {
        MMU_free_page(start_address);
        start_address = (void *)(((virtual_addr_t)start_address) + PAGE_SIZE);
    }
}