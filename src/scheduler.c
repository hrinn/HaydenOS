#include "scheduler.h"
#include <stddef.h>
#include "printk.h"
#include "proc_queue.h"

static proc_queue_t queue;
static process_t *next;

// Adds a thread to the schedule
void sched_admit(process_t *thread) {
    if (queue.head == NULL) {
        next = thread;
    }
    append_proc(thread, &queue);
}

// Selects the next thread to run
process_t *rr_next() {
    process_t *temp;

    if (next == NULL) return NULL;

    temp = next;
    
    if (next == queue.tail) {
        next = queue.head;
    } else {
        next = next->next;
    }

    return temp;
}

// Removes a thread from the schedule
void sched_remove(process_t *thread) {
    if (thread == next) {
        if (thread == queue.tail && thread == queue.head) {
            next = NULL;
        } else if (thread == queue.tail) {
            next = queue.head;
        } else {
            next = thread->next;
        }
    }
    remove_proc(thread, &queue);
}

// Returns a pointer to the next thread
process_t *rr_peek() {
    return next;
}