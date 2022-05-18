#include "part.h"
#include "block.h"
#include "printk.h"
#include "kmalloc.h"
#include "string.h"
#include <stdint-gcc.h>

#define BLOCK_SIZE 512
#define FAT32_LBA_TYPE 0xC

typedef struct part_entry {
    uint8_t status;
    uint8_t chs_addr_start[3];
    uint8_t type;
    uint8_t chs_addr_end[3];
    uint32_t lba_addr;
    uint32_t num_sectors;
} part_entry_t;

void print_part(part_entry_t *part, int n) {
    if (part->type == 0) {
        printb("Partition %d - None\n", n);
    } else {
        printb("Partition %d - Type: 0x%X, Size: %d, Start LBA: 0x%x\n", 
            n, part->type, part->num_sectors, part->lba_addr);
    }
}

int part_read_block(block_dev_t *dev, uint64_t blk_num, void *dst) {
    part_block_dev_t *part = (part_block_dev_t *)dev;

    if (blk_num >= part->num_sectors) {
        printk("part_read_block(): Tried to read past end of partition\n");
        return -1;
    }

    return ATA_read_block(dev, blk_num + part->lba_offset, dst);
}

// Parses the master boot record on the primary master drive
// Places partition devices 
// Returns 1 on success, -1 on failure
int parse_MBR(ATA_block_dev_t *drive, part_block_dev_t **partitions) {
    uint8_t block[BLOCK_SIZE];
    part_entry_t *part;
    part_block_dev_t *dev;
    char *part_name;
    int i = 0, len;

    printk("Parsing MBR on %s\n", drive->dev.name);

    // Read the first block (MBR)
    drive->dev.read_block((block_dev_t *)drive, 0, block);

    // Validate the boot signature
    if (block[510] != 0x55 || block[511] != 0xAA) {
        printk("parse_MBR(): failed to validate boot signature\n");
        return -1;
    }
    
    // Parse partitions
    part = (part_entry_t *)(&block[446]);
    for (i = 0; i < 4; i++) {
        print_part(part, i);

        if (part->type != FAT32_LBA_TYPE) {
            partitions[i] = NULL;
            part++;
            continue;
        }

        // Create and register a partition block device
        dev = (part_block_dev_t *)kmalloc(sizeof(part_block_dev_t));
        memcpy(dev, drive, sizeof(ATA_block_dev_t));
        dev->lba_offset = part->lba_addr;
        dev->num_sectors = part->num_sectors;
        dev->ata.dev.type = PARTITION;
        dev->ata.dev.read_block = VSPACE(part_read_block);

        // Set partition name
        len = strlen(drive->dev.name);
        part_name = (char *)kmalloc(len + 1);
        memcpy(part_name, drive->dev.name, len);
        part_name[len] = (char)('0' + i);
        dev->ata.dev.name = part_name;

        BLK_register((block_dev_t *)dev);

        partitions[i] = dev;
        part++;
    }

    return 1;
}