#ifndef PROC_H
#define PROC_H

#include <stdint-gcc.h>
#include "memdef.h"
#include "proc_queue.h"

typedef void (*kproc_t)(void *);

#define KEXIT_IRQ 207
#define KEXIT_IST 4

// Invokes the scheduler and passes control to the next eligible thread
static inline void yield(void) {
    asm volatile ( "INT $206" );
}

static inline void kexit(void) {
    asm volatile ( "INT $207" );
}

struct regfile {
    uint64_t rax;   uint64_t rbx;   uint64_t rcx;
    uint64_t rdx;   uint64_t rdi;   uint64_t rsi;
    uint64_t r8;    uint64_t r9;    uint64_t r10;
    uint64_t r11;   uint64_t r12;   uint64_t r13;
    uint64_t r14;   uint64_t r15;
    uint16_t cs;    uint16_t ss;    uint16_t ds;
    uint16_t es;    uint16_t fs;    uint16_t gs;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t rip;
    uint64_t rflags;
};

typedef struct Process process_t;
struct Process {
    struct regfile regfile;
    int pid;
    virtual_addr_t stack_top;
    process_t *next;
    process_t *prev;
};

extern process_t *curr_proc;
extern process_t *next_proc;

// Basic process management
void PROC_init(void);
void PROC_run(void);
process_t *PROC_create_kthread(kproc_t entry_point, void *arg);

// Blocking process management
// void PROC_block_on(proc_queue_t *, int enable_ints);
// void PROC_unblock_all(proc_queue_t *);
// void PROC_unblock_head(proc_queue_t *);
void PROC_init_queue(proc_queue_t *);

#endif