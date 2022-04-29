#include "scheduler.h"
#include <stddef.h>
#include "printk.h"

static kthread_context_t *head;
static kthread_context_t *tail;
static kthread_context_t *next;

void print_queue() {
    kthread_context_t *current = head;

    while (current != NULL) {
        printk("%p\n", (void *)current);
        current = current->sched_next;
    }
}

// Adds a thread to the schedule
void rr_admit(kthread_context_t *thread) {
    if (thread == NULL) return;

    if (head == NULL) {
        head = thread;
        tail = thread;
        next = thread;
    } else {
        tail->sched_next = thread;
        tail = thread;
    }
}

// Removes a thread from the schedule
void rr_remove(kthread_context_t *thread) {
    kthread_context_t *prev = NULL;
    kthread_context_t *current;

    if (thread == NULL) return;

    // Locate target thread
    current = head;
    while (current != thread && current != NULL) {
        prev = current;
        current = current->sched_next;
    }

    if (current != thread) return; // Thread was not scheduled

    if (prev != NULL) {
        prev->sched_next = current->sched_next;
    } else {
        // We are removing the head
        head = current->sched_next;
    }

    if (current->sched_next == NULL) {
        // We are removing the tail
        tail = prev;
    }

    if (current == next) {
        next = current->sched_next;
    }
}

// Returns a pointer to the next thread
kthread_context_t *rr_peek() {
    return next;
}

// Selects the next thread to run
kthread_context_t *rr_next() {
    kthread_context_t *temp;

    if (next == NULL) return NULL;

    temp = next;
    
    if (next->sched_next != NULL) {
        next = next->sched_next;
    } else {
        next = head;
    }

    return temp;
}