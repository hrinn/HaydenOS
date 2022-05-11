#include "block.h"
#include <stddef.h>

static block_dev_t *dev_head;

// Insert dev into dev list
// Returns 1 on success, -1 on failure
int BLK_register(block_dev_t *dev) {
    if (dev_head == NULL) {
        dev_head = dev;
        return 1;
    }

    // Find end of device list
    block_dev_t *current = dev_head;

    while (current->next != NULL) {
        current = current->next;
    }

    current->next = dev;
    return 1;
}