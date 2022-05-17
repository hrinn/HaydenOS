#include "block.h"
#include "ll_generic.h"
#include <stddef.h>

static block_dev_t *head, *tail;

// Insert dev into dev list
// Returns 1 on success, -1 on failure
int BLK_register(block_dev_t *dev) {
    LL_APPEND(head, tail, dev);
    return 1;
}