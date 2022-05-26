#include "sys_call.h"
#include "memdef.h"
#include "mmu.h"
#include "irq.h"
#include <stddef.h>
#include "gdt.h"
#include "printk.h"
#include "sys_call_ints.h"
#include "printk.h"

#define NUM_SYS_CALLS 256

sys_call_f sys_calls[NUM_SYS_CALLS];

void set_sys_call(uint8_t num, sys_call_f sys_call) {
    sys_calls[num] = VSPACE(sys_call);
}

void init_sys_calls() {
    virtual_addr_t stack_top = allocate_thread_stack();
    TSS_set_ist(stack_top, SYS_CALL_IST);
    set_sys_call(PUTC_SYS_CALL, putc_sys_call);
}

uint64_t sys_call_isr(uint64_t sys_call_index, uint64_t arg) {
    return sys_calls[sys_call_index](arg);
}