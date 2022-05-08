#ifndef BLOCK_DEV_H
#define BLOCK_DEV_H

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

typedef struct ata_block_dev ata_block_dev_t;
struct ata_block_dev {
    block_dev_t dev;
    uint16_t ata_base, ata_master;
    uint8_t slave, irq;
    struct ATA_request *req_head, *req_tail;
};

ata_block_dev_t *ATA_probe(uint16_t base, uint16_t master, 
    uint8_t slave, const char *name, uint8_t irq);

// PIO Bus Addresses
#define PRIMARY_BASE 0x1F0
#define SECONDARY_BASE 0x170

// IRQ
#define PRIMARY_INT_LINE 14
#define SECONDARY_INT_LINE 15
#define PRIMARY_IRQ PRIMARY_INT_LINE + 32
#define SECONDARY_IRQ SECONDARY_INT_LINE + 32

#endif