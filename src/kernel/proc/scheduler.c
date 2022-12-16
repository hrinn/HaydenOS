#include "scheduler.h"
#include <stddef.h>
#include "printk.h"
#include "proc_queue.h"

static proc_queue_t queue;
static process_t *current;

// Adds a thread to the schedule
void sched_admit(process_t *thread) {
    append_proc(thread, &queue);
}

// Selects the next thread to run in a round robin scheduler
process_t *rr_next() {
    // Iterate and return current
    if (current == NULL || current == queue.tail) {
        current = queue.head;
    } else {
        current = current->next;
    }

    return current;
}

// Selects the next thread to run in a FIFO scheduler
process_t *fifo_next() {
    if (current == NULL) {
        current = queue.head;
    }

    return current;
}

process_t *lifo_next() {
    if (current == NULL) {
        current = queue.tail;
    }
    return current;
}

// Removes a thread from the schedule
void sched_remove(process_t *thread) {
    if (thread == current) {
        current = thread->prev;
    }
    remove_proc(thread, &queue);
}

bool are_procs_scheduled() {
    return queue.head != NULL;
}