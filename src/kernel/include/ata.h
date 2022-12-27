#ifndef ATA_H
#define ATA_H

#include "block.h"
#include <stdbool.h>

typedef struct ATA_block_dev ATA_block_dev_t;

struct ATA_block_dev {
    block_dev_t dev;
    uint16_t ata_base;
    uint8_t slave;
};

ATA_block_dev_t *ATA_probe(uint16_t base, uint8_t slave, const char *name, uint8_t irq);
int ATA_read_block(block_dev_t *dev, uint64_t blk_num, void *dst);

// PIO Bus Addresses
#define PRIMARY_BASE 0x1F0
#define SECONDARY_BASE 0x170

// IRQ
#define PRIMARY_IRQ 46
#define SECONDARY_IRQ 47

#endif