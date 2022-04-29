#include "proc.h"
#include "irq.h"
#include "mmu.h"
#include "memdef.h"
#include <stddef.h>
#include "kmalloc.h"
#include "scheduler.h"
#include "gdt.h"
#include "printk.h"

#define YIELD_IRQ 206
#define IE_FLAG 0x200
#define RES_FLAG 0x2

void yield_isr(uint8_t, unsigned int, void *);
void kexit_isr(uint8_t, unsigned int, void *);

static kthread_context_t *orig_proc;
kthread_context_t *curr_proc;
kthread_context_t *next_proc;

// Initializes the multitasking system
void PROC_init(void) {
    // Setup a stack for the kexit ISR in the TSS
    TSS_set_ist(allocate_thread_stack(), KEXIT_IST);
    IRQ_set_handler(YIELD_IRQ, yield_isr, NULL);
    IRQ_set_handler(KEXIT_IRQ, kexit_isr, NULL);
}

// Drives the multitasking system
void PROC_run(void) {
    uint16_t int_en = check_int();
    if (int_en) cli();
    curr_proc = NULL;
    next_proc = NULL;

    if (rr_peek() == NULL) {
        // There were no threads in the scheduler
        return;
    }

    // Setup an original process and set it as the current process
    // On interrupt, the current process's context will be saved
    orig_proc = (kthread_context_t *)kmalloc(sizeof(kthread_context_t));
    // Maybe need to set rbp??
    curr_proc = orig_proc;

    if (int_en) sti();
    yield();

    // Return to here means all threads have exited
    kfree(orig_proc);
}

void PROC_reschedule(void) {
    next_proc = rr_next();

    if (next_proc == NULL) {
        next_proc = orig_proc;
    }
}

// Adds a new thread to the multitasking system
void PROC_create_kthread(kproc_t entry_point, void *arg) {
    kthread_context_t *context = (kthread_context_t *)kcalloc(1, sizeof(kthread_context_t));

    context->stack_top = allocate_thread_stack();

    // Set the registers that are currently known
    context->regfile.rbp = context->stack_top;
    context->regfile.rsp = context->stack_top;
    context->regfile.rip = ((uint64_t)entry_point) + KERNEL_TEXT_START;
    context->regfile.rdi = (uint64_t)arg;
    context->regfile.cs = KERNEL_CODE_OFFSET;
    context->regfile.rflags |= (IE_FLAG | RES_FLAG);

    // Add this context to the scheduler
    rr_admit(context);
}

// Invokes the scheduler and passes control to the next eligible thread
void yield_isr(uint8_t irq, unsigned int error_code, void *arg) {
    PROC_reschedule();
}

// Exits and destroys the state of the caller thread
void kexit_isr(uint8_t irq, unsigned int error_code, void *arg) {
    // Deallocate the stack
    free_thread_stack(curr_proc->stack_top);

    // Deschedule the thread
    rr_remove(curr_proc);

    // Deallocate the thread context
    kfree(curr_proc);

    // Runs the scheduler to pick another process
    PROC_reschedule();
}