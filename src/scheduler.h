#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "proc.h"

void sched_admit(process_t *thread);
void sched_remove(process_t *thread);
process_t *rr_next(void);
process_t *rr_peek();
process_t *fifo_peek();
process_t *fifo_next();

#endif