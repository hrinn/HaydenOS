#include "proc.h"
#include "irq.h"
#include "mmu.h"
#include "memdef.h"
#include <stddef.h>
#include "kmalloc.h"
#include "scheduler.h"
#include "gdt.h"
#include "printk.h"

#define IE_FLAG 0x200
#define RES_FLAG 0x2

void yield_sys_call();
void kexit_sys_call();

static int pid = 1;
static process_t orig_proc;
process_t *curr_proc;
process_t *next_proc;

// Initializes the multitasking system
void PROC_init(void) {
    set_sys_call(YIELD_SYS_CALL, yield_sys_call);
    set_sys_call(KEXIT_SYS_CALL, kexit_sys_call);
}

// Drives the multitasking system
void PROC_run(void) {
    uint16_t int_en = check_int();
    if (int_en) cli();
    curr_proc = &orig_proc;
    next_proc = &orig_proc;

    if (!are_procs_scheduled()) {
        if (int_en) sti();
        return;
    }

    if (int_en) sti();
    // This yield runs on its own stack so it will first generated a page fault
    // Returning from this page fault 
    yield();
}

void PROC_reschedule(void) {
    next_proc = rr_next();

    if (next_proc == NULL) {
        next_proc = &orig_proc;
    }
}

void kthread_wrapper(kproc_t entry_point, void *arg) {
    entry_point(arg);
    kexit();
}

// Adds a new thread to the multitasking system
struct Process *PROC_create_kthread(kproc_t entry_point, void *arg) {
    process_t *context = (process_t *)kcalloc(1, sizeof(process_t));

    context->stack_top = allocate_thread_stack();

    // Set the registers that are currently known
    context->pid = pid++;
    context->regfile.rbp = context->stack_top;
    context->regfile.rsp = context->stack_top;
    context->regfile.rip = ((uint64_t)kthread_wrapper) + KERNEL_TEXT_START;
    context->regfile.rdi = ((uint64_t)entry_point) + KERNEL_TEXT_START;
    context->regfile.rsi = (uint64_t)arg;
    context->regfile.cs = KERNEL_CODE_OFFSET;
    context->regfile.ss = 0;
    context->regfile.rflags |= (IE_FLAG | RES_FLAG);

    // Add this context to the scheduler
    sched_admit(context);
    return context;
}

// Invokes the scheduler and passes control to the next eligible thread
void yield_sys_call() {
    PROC_reschedule();
}

// Exits and destroys the state of the caller thread
void kexit_sys_call() {
    // Deallocate the stack
    free_thread_stack(curr_proc->stack_top);

    // Deschedule the thread
    sched_remove(curr_proc);

    // Deallocate the thread context
    kfree(curr_proc);

    // Runs the scheduler to pick another process
    PROC_reschedule();
}

// Blocking process management

void PROC_block_on(proc_queue_t *queue, int enable_ints) {
    if (!queue) return;

    sched_remove(curr_proc);   // Deschedule the current proc
    append_proc(curr_proc, queue);
    if (enable_ints) STI;

    yield(); // Context switch
}

void PROC_unblock_all(proc_queue_t *queue) {
    process_t *current;

    if (!queue) return;

    while ((current = pop_proc(queue)) != NULL) {
        sched_admit(current);
    }

    yield(); // Context switch
}

void PROC_unblock_head(proc_queue_t *queue) {
    process_t *current;

    if (!queue) return;

    current = pop_proc(queue);
    if (current != NULL) sched_admit(current);

    yield(); // Context switch
}

void PROC_init_queue(proc_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}