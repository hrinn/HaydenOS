#ifndef PROC_QUEUE_H
#define PROC_QUEUE_H

typedef struct Process process_t;

typedef struct proc_queue {
    process_t *head;
    process_t *tail;
} proc_queue_t;

void remove_proc(process_t *proc, proc_queue_t *queue);
void append_proc(process_t *proc, proc_queue_t *queue);
process_t *pop_proc(proc_queue_t *queue);
void print_queue(proc_queue_t *queue);

#endif