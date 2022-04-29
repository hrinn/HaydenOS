#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "proc.h"

void rr_admit(process_t *thread);
void rr_remove(process_t *thread);
process_t *rr_next(void);
void print_queue();
process_t *rr_peek();

#endif