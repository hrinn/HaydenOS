#include "stack_alloc.h"
#include <stddef.h>
#include "page_table.h"
#include "kmalloc.h"

typedef struct free_thread_stack {
    virtual_addr_t top;
    struct free_thread_stack *next;
} free_thread_stack_t;

static virtual_addr_t thread_stack_brk = KERNEL_STACKS_START;
static free_thread_stack_t *free_thread_stacks_head;

// Demand allocates a 2 page stack in the thread stack region of virtual memory
// Returns the addresses of the top of the stack
virtual_addr_t allocate_thread_stack() {
    virtual_addr_t vcurrent;
    virtual_addr_t start = thread_stack_brk;

    // Check if there is a free thread stack
    if (free_thread_stacks_head != NULL) {
        start = free_thread_stacks_head->top - STACK_SIZE - PAGE_SIZE;
        kfree(free_thread_stacks_head);
        free_thread_stacks_head = free_thread_stacks_head->next;
    } else {
        thread_stack_brk += STACK_SIZE + PAGE_SIZE;
    }

    // Leave room for a guard page
    for (vcurrent = start + PAGE_SIZE; 
        vcurrent < start + STACK_SIZE + PAGE_SIZE; 
        vcurrent += PAGE_SIZE)
    {
        map_page(vcurrent, 0, PAGE_ALLOCATED | PAGE_WRITABLE | PAGE_NO_EXECUTE);
    }

    return vcurrent;
}

void free_thread_stack(virtual_addr_t top) {
    virtual_addr_t vcurrent;
    free_thread_stack_t *new;

    for (vcurrent = top - PAGE_SIZE; 
        vcurrent > top - PAGE_SIZE - STACK_SIZE; 
        vcurrent -= PAGE_SIZE)
    {
        free_pf_from_virtual_addr(vcurrent);
    }

    // Add top to the list of free stacks
    new = (free_thread_stack_t *)kmalloc(sizeof(free_thread_stack_t));
    new->top = top;
    new->next = NULL;

    if (free_thread_stacks_head == NULL) {
        free_thread_stacks_head = new;
    } else {
        new->next = free_thread_stacks_head;
        free_thread_stacks_head = new;
    }
}