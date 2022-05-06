#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "proc.h"
#include <stdbool.h>

void sched_admit(process_t *thread);
void sched_remove(process_t *thread);
process_t *rr_next(void);
process_t *fifo_next(void);
bool are_procs_scheduled();
process_t *lifo_next();

#endif