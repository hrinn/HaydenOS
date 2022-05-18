#include "sys_call.h"
#include "memdef.h"
#include "mmu.h"
#include "irq.h"
#include <stddef.h>
#include "gdt.h"

#define NUM_SYS_CALLS 256

sys_call_f sys_calls[NUM_SYS_CALLS];
uint8_t sys_call_index;

void sys_call_isr(uint8_t, unsigned int, void *);

void init_sys_calls() {
    // Preallocate the sys call stack
    virtual_addr_t stack_top = allocate_thread_stack();
    virtual_addr_t stack_bottom = stack_top - (PAGE_SIZE * 2);
    uint64_t *writeable_stack = (uint64_t *)stack_bottom;

    // Write to the stack now to trigger a page fault
    // So that it doesn't happen when a system call occurs
    // Because that's weird
    *writeable_stack = 0;

    TSS_set_ist(stack_top, SYS_CALL_IST);
    IRQ_set_handler(SYS_CALL_IRQ, sys_call_isr, NULL);
}

void set_sys_call(uint8_t num, sys_call_f sys_call) {
    sys_calls[num] = VSPACE(sys_call);
}

void sys_call_isr(uint8_t irq, unsigned int error_code, void *arg) {
    sys_calls[sys_call_index]();
}