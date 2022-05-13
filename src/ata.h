#ifndef ATA_H
#define ATA_H

#include "block.h"
#include "proc.h"

typedef struct ATA_request {
    struct ATA_request *next;
    void *dst;
} ATA_request_t;

typedef struct ATA_block_dev {
    block_dev_t dev;
    uint16_t ata_base, ata_master;
    uint8_t slave, irq;
    struct ATA_request *req_head, *req_tail;
    proc_queue_t blocked_queue;
} ATA_block_dev_t;

ATA_block_dev_t *ATA_probe(uint16_t base, uint16_t master, 
    uint8_t slave, const char *name, uint8_t irq);
int ATA_read_block(block_dev_t *dev, uint64_t blk_num, void *dst);

// PIO Bus Addresses
#define PRIMARY_BASE 0x1F0
#define SECONDARY_BASE 0x170

// IRQ
#define PRIMARY_IRQ 46
#define SECONDARY_IRQ 47

#endif