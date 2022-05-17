#include "vfs.h"
#include "ll_generic.h"
#include <stddef.h>

static superblock_t *head, *tail;

// Probes a block device to determine if a valid filesystem exists on it
// Registers the filesystem with the VFS if it does
void FS_register(FS_detect_cb probe, block_dev_t *dev) {
    superblock_t *superblock = probe(dev);
    LL_APPEND(head, tail, superblock);
}