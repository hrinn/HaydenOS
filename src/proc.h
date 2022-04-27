#ifndef PROC_H
#define PROC_H

typedef void (*kproc_t)(void *);

void PROC_run(void);
void PROC_create_kthread(kproc_t entry_point, void *arg);
void PROC_reschedule(void);
void kexit(void);

// Invokes the scheduler and passes control to the next eligible thread
static inline void yield(void) {
    asm volatile ( "INT $206" );
}

static inline void kexit(void) {
    asm volatile ( "INT $207" );
}

#endif