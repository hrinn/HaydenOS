#include "vfs.h"
#include "ll_generic.h"
#include "kmalloc.h"
#include "printk.h"
#include <stddef.h>
#include "string.h"
#include "memdef.h"

typedef struct FS_impl {
    FS_detect_cb probe;
    struct FS_impl *next;
} FS_impl_t;

static FS_impl_t *head, *tail;

inode_t *FS_inode_for_path_helper(char *path, inode_t *cwd);

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

int copy_path_item(char *path, char *dst) {
    int i = 0;

    for (i = 0; path[i] && path[i] != '/'; i++) {
        dst[i] = path[i];
    }
    dst[i] = 0;

    return i;
}

typedef struct path_search {
    char *name;
    inode_t *result;
} path_search_t;

void replace_slashes_with_null(char *str) {
    while (*str) {
        if (*str == '/') *str = 0;
        str++;
    }
}

int path_cb(const char *ent_name, inode_t *inode, void *p) {
    path_search_t *path_search = (path_search_t *)p;

    if (strcmp(ent_name, path_search->name) == 0) {
        path_search->result = inode;
    } else {
        inode->free(&inode);
    }
    return 1;
}

// Walks directory trees
inode_t *FS_inode_for_path(char *path, inode_t *cwd) {
    path_search_t path_search;
    char path_item[255];
    int offset;

    if (path[0] == '/') {
        path++;
        cwd = cwd->parent_superblock->root_inode;
    }

    if (*path == 0) return cwd;

    while (*path) {
        // Get single entry
        offset = copy_path_item(path, path_item);
        path_search.name = path_item;

        // Read directories, store result
        if ((cwd->st_mode & S_IFDIR) == 0) {
            return NULL;
        }

        cwd->readdir(cwd, VSPACE(path_cb), (void *)&path_search);

        // Handle no result
        if (path_search.result == NULL) {
            return NULL;
        }

        // Check if result is final
        if (*(path + offset) == 0) {
            break;
        }

        // Iterate to next level of directory, and next path item
        cwd = path_search.result;
        path += offset + 1;
    }

    return path_search.result;
}