#include "vfs.h"
#include "ll_generic.h"
#include "kmalloc.h"
#include "printk.h"
#include <stddef.h>

typedef struct FS_impl {
    FS_detect_cb probe;
    struct FS_impl *next;
} FS_impl_t;

static FS_impl_t *head, *tail;

// Registers a FS implementation with the VFS
void FS_register(FS_detect_cb probe) {
    FS_impl_t *FS_impl = (FS_impl_t *)kmalloc(sizeof(FS_impl_t));
    FS_impl->probe = probe;
    LL_APPEND(head, tail, FS_impl);
}

// Probes a block device to determine if a valid filesystem exists on it
// Registers the filesystem with the VFS if it does
superblock_t *FS_probe(block_dev_t *dev) {
    FS_impl_t *impl = head;
    superblock_t *sb;

    printk("Probing for supported filesystem on %s\n", dev->name);

    while (impl != NULL) {
        if ((sb = impl->probe(dev)) != NULL) {
            return sb;
        }
    }

    printk("FS_probe(): Failed to find supported filesystem\n");

    return NULL;
}

// Walks directory trees
inode_t *FS_inode_for_path(const char *path, inode_t *cwd) {
    // Parse out first component of path

    // Call readdir on the inode, passing in the entry name to match

    // In the callback function, if a match is found, recursively call readdir on the next inode

    // Repeat
    return NULL;
}