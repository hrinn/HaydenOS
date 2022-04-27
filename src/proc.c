#include "proc.h"
#include "irq.h"
#include "mmu.h"
#include <stdint-gcc.h>
#include <stddef.h>
#include "memdef.h"
#include "kmalloc.h"

#define YIELD_IRQ 206
#define KEXIT_IRQ 207

void yield_isr(uint8_t, unsigned int, void *);

struct regfile {
    uint64_t rax;   uint64_t rbx;   uint64_t rcx;   uint64_t rdx;   uint64_t rdi;   uint64_t rsi;
    uint64_t r8;    uint64_t r9;    uint64_t r10;   uint64_t r11;   uint64_t r12;   uint64_t r13;   uint64_t r14;   uint64_t r15;
    uint64_t cs;    uint64_t ss;    uint64_t ds;    uint64_t es;    uint64_t fs;    uint64_t gs;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t flags;
};

typedef struct kthread_context {
    virtual_addr_t stack_top;
    struct regfile regfile;
    struct kthread_context *rr_next;
} kthread_context_t;

static kthread_context_t *current_process;
static kthread_context_t *next_process;

// Adds a thread to the schedule
static void schedule(kthread_context_t *thread) {
    kthread_context_t *node;

    if (next_process == NULL) {
        next_process = thread;
    } else {
        node = next_process;
        while (node->rr_next != NULL) node = node->rr_next;
        node->rr_next = thread;
    }
}

// Removes a thread from the schedule
static void deschedule(kthread_context_t *thread) {
    
}

// Initializes the multitasking system
void PROC_init(void) {
    IRQ_set_handler(YIELD_IRQ, yield_isr, NULL);
    IRQ_set_handler(KEXIT_IRQ, kexit, NULL);
}

// Drives the multitasking system
// Should be called in a loop at the end of kmain
void PROC_run(void) {
    if (next_process == NULL) return;

    // Select the next thread to run
}

// Adds a new thread to the multitasking system
void PROC_create_kthread(kproc_t entry_point, void *arg) {
    kthread_context_t *context = (kthread_context_t *)kcalloc(1, sizeof(kthread_context_t));

    context->stack_top = allocate_thread_stack();

    // Set the registers that are currently known
    context->regfile.rbp = context->stack_top;
    context->regfile.rsp = context->stack_top;
    context->regfile.rip = entry_point;
    context->regfile.rdi = (uint64_t)arg;

    // Add this context to the scheduler
    schedule(context);
}

// Selects next thread to run
void PROC_reschedule(void) {
    // selects the thread that called proc_run if no other threads are available
    // does not actually perform the context switch
}

// Invokes the scheduler and passes control to the next eligible thread
void yield_isr(uint8_t irq, unsigned int error_code, void *arg) {
    // Save current set of register values into current process

    // Load registers in next processors into current set of registers

    // Set current process to next process
}

// Exits and destroys the state of the caller thread
void kexit_isr(uint8_t irq, unsigned int error_code, void *arg) {
    // TODO: Use IST to put kexit on another stack

    // Deallocate the stack

    // Runs the scheduler to pick another process
}