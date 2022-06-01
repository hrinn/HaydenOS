#ifndef BLOCK_H
#define BLOCK_H

#include <stdint-gcc.h>

enum block_dev_type {MASS_STORAGE, PARTITION};
typedef struct block_dev block_dev_t;
typedef int (*read_block_f)(block_dev_t *dev, uint64_t blk_num, void *dst);

struct block_dev {
    uint64_t tot_len;
    read_block_f read_block;
    uint32_t blk_size;
    enum block_dev_type type;
    const char *name;
    uint8_t fs_type;
    block_dev_t *next;
};

int BLK_register(block_dev_t *dev);

#endif