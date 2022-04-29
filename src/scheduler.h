#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "proc.h"

void rr_admit(kthread_context_t *thread);
void rr_remove(kthread_context_t *thread);
kthread_context_t *rr_next(void);
void print_queue();
kthread_context_t *rr_peek();

#endif