#ifndef VFS_H
#define VFS_H

#include "block.h"
#include <stdint-gcc.h>

typedef uint64_t ino_t;
typedef uint64_t mode_t;
typedef uint64_t uid_t;
typedef uint64_t gid_t;
typedef uint64_t off_t;

typedef struct inode inode_t;
typedef struct file file_t;
typedef struct superblock superblock_t;

// mode_t values
#define S_IFDIR 0040000 // Directory
#define S_IFREG 0100000 // Regular file

typedef int (*readdir_cb)(const char *, inode_t *, void *);

struct file {
    superblock_t *superblock;
    off_t cursor;
    unsigned long first_cluster;
    int (*close)(file_t **file);
    int (*read)(file_t *file, char *dst, int len);
    int (*write)(file_t *file, char *dst, int len);
    int (*lseek)(file_t *file, off_t offset);
    int (*mmap)(file_t *file, void *addr);
};

struct inode {
    ino_t st_ino;
    mode_t st_mode;
    uid_t st_uid;
    gid_t st_gid;
    off_t st_size;
    file_t *(*open)(inode_t *inode);
    int (*readdir)(inode_t *inode, readdir_cb callback, void *p);
    int (*unlink)(inode_t *inode, const char *name);
    void (*free)(inode_t **inode);
    inode_t *parent_inode;
    superblock_t *parent_superblock;
};

struct superblock {
    inode_t *root_inode;
    inode_t *(*read_inode)(superblock_t *, unsigned long inode_num);
    int (*sync_fs)(superblock_t *);
    void (*put_super)(superblock_t *);
    const char *name, *type;
    block_dev_t *dev;
};

typedef superblock_t *(*FS_detect_cb)(struct block_dev *dev);

void FS_register(FS_detect_cb probe);
superblock_t *FS_probe(block_dev_t *dev);
inode_t *FS_inode_for_path(char *path, inode_t *cwd);

#endif