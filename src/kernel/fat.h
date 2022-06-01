#ifndef FAT_H
#define FAT_H

#include "block.h"
#include "vfs.h"

superblock_t *FAT_detect(block_dev_t *dev);

#endif