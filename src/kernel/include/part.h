#ifndef PART_H
#define PART_H

#include "ata.h"

typedef struct part_block_dev {
    ATA_block_dev_t ata;
    uint32_t lba_offset;
    uint32_t num_sectors;
} part_block_dev_t;

int parse_MBR(ATA_block_dev_t *, part_block_dev_t **);

#endif