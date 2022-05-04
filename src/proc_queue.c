#include "proc_queue.h"
#include <stddef.h>
#include "printk.h"
#include "proc.h"

void print_queue(proc_queue_t *queue) {
    process_t *current = queue->head;
    while (current != NULL) {
        printk("%p,", (void *)current);
        current = current->next;
    }
    printk("\n");
}

// Removes a process from the process queue
void remove_proc(process_t *proc, proc_queue_t *queue) {
    process_t *prev = proc->prev;
    process_t *next = proc->next;

    // Unlink
    if (prev != NULL) prev->next = next;
    if (next != NULL) next->prev = prev;

    // Adjust head and tail
    if (queue->head == proc) queue->head = next;
    if (queue->tail == proc) queue->tail = prev;

    proc->next = NULL;
    proc->prev = NULL;
}

// Appends a process to the process queue
void append_proc(process_t *proc, proc_queue_t *queue) {
    if (queue->head == NULL) {
        queue->head = proc;
        queue->tail = proc;
        return;
    }

    queue->tail->next = proc;
    proc->prev = queue->tail;
    queue->tail = proc;
}

// Removes and returns the head of the process queue
process_t *pop_proc(proc_queue_t *queue) {
    process_t *temp;

    if (queue->head == NULL) return NULL;

    temp = queue->head;
    queue->head = temp->next;
    queue->head->prev = NULL;

    temp->next = NULL;
    temp->prev = NULL;

    return temp;
}