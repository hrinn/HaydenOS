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
    sys_calls[num] = sys_call;
}

void init_sys_calls() {
    set_sys_call(PUTC_SYS_CALL, putc_sys_call);
}

uint64_t sys_call_isr(uint64_t sys_call_index, uint64_t arg) {
    sys_call_f sys_call = sys_calls[sys_call_index];
    if (sys_call != NULL) {
        return sys_call(arg);
    }
    printk("Invalid syscall %ld!\n", sys_call_index);
    return 0;
}