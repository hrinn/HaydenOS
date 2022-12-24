#include "ata.h"
#include <stddef.h>
#include "ioport.h"
#include "printk.h"
#include "kmalloc.h"
#include "memdef.h"
#include "error.h"

// PIO Registers
#define DATA_REG(base) base + 0         // R/W (16 bit)
#define ERROR_REG(base) base + 1        // R
#define FEAT_REG(base) base + 1         // W
#define SEC_CNT_REG(base) base + 2      // R/W
#define SEC_NUM_REG(base) base + 3      // R/W
#define CYL_LOW_REG(base) base + 4      // R/W
#define CYL_HIGH_REG(base) base + 5     // R/W
#define DRIVE_HEAD_REG(base) base + 6   // R/W
#define STAT_REG(base) base + 7         // R
#define CMD_REG(base) base + 7          // W
#define ALT_STAT_REG(base) base + 0x206
#define DEV_CTRL_REG(base) base + 0x206
#define DEV_ADDR_REG(base) base + 0x207

// Register bits
#define DEV_CTRL_NIEN 0x2
#define STAT_DRQ (1 << 3)
#define STAT_BSY (1 << 7)
#define STAT_ERR 1

// Values
#define FLOATING_BUS 0xFF

// Commands
#define CMD_SELECT_MASTER 0xA0
#define CMD_SELECT_SLAVE 0xB0
#define CMD_IDENTIFY 0xEC
#define CMD_READ_SECTORS_EXT 0x24

void ATA_poll(uint16_t base) {
    int i;
    uint8_t status;

    for (i = 0; i < 4; i++) inb(ALT_STAT_REG(base));

retry:
    status = inb(ALT_STAT_REG(base));
    if (status & STAT_BSY) goto retry;

retry2:
    status = inb(ALT_STAT_REG(base));
    if (status & STAT_ERR) {
        panic("ATA_poll(): Error status while polling!\n");
    }

    if (!(status & STAT_DRQ)) goto retry2;

    return;
}

int ATA_read_block(block_dev_t *dev, uint64_t blk_num, void *dst) {
    ATA_block_dev_t *ata_dev = (ATA_block_dev_t *)dev;
    uint16_t *block_dst = (uint16_t *)dst;
    uint8_t *lba_n = (uint8_t *)&blk_num, status;
    int i;
    
    if (blk_num >= dev->tot_len) {
        printk("ATA_read_block(): Tried to read past end of drive\n");
        return -1;
    }

    // Select master / slave
    outb(DRIVE_HEAD_REG(ata_dev->ata_base), 0x40 | (ata_dev->slave << 4));

    // Send address
    outb(SEC_CNT_REG(ata_dev->ata_base), 0); 
    outb(SEC_NUM_REG(ata_dev->ata_base), lba_n[3]);
    outb(CYL_LOW_REG(ata_dev->ata_base), lba_n[4]);
    outb(CYL_HIGH_REG(ata_dev->ata_base), lba_n[5]);
    outb(SEC_CNT_REG(ata_dev->ata_base), 1);
    outb(SEC_NUM_REG(ata_dev->ata_base), lba_n[0]);
    outb(CYL_LOW_REG(ata_dev->ata_base), lba_n[1]);
    outb(CYL_HIGH_REG(ata_dev->ata_base), lba_n[2]);

    // Send read command
    outb(CMD_REG(ata_dev->ata_base), CMD_READ_SECTORS_EXT);

    // Poll until data is ready
    ATA_poll(ata_dev->ata_base);

    // Read 256 16 bit values from data port, into request's dst
    for (i = 0; i < 256; i++) {
        block_dst[i] = inw(DATA_REG(ata_dev->ata_base));
    }

    // Ensure status is good after handling read
    status = inb(ALT_STAT_REG(ata_dev->ata_base));
    if (status & STAT_BSY || status & STAT_DRQ || status & STAT_ERR) {
        printk("ATA_read_block(): bad status: %x\n", status);
        return -1;
    }

    return 1;
}

// Ensures specified controller is present
// Returns a pointer to a struct with its information
ATA_block_dev_t *ATA_probe(uint16_t base, uint8_t slave, const char *name, uint8_t irq) 
{
    uint8_t status, command;
    uint16_t data[256];
    uint64_t sectors = 0;
    ATA_block_dev_t *ata_dev;
    int i;

    // Turn off interrupts
    outb(DEV_CTRL_REG(base), 0x2);

    // Floating bus detection
    status = inb(STAT_REG(base));
    if (status == FLOATING_BUS) {
        printb("ata_probe(): No disks on IDE bus\n");
        return NULL;
    }

    // Setup for identify command
    command = (slave) ? CMD_SELECT_SLAVE : CMD_SELECT_MASTER;
    outb(DRIVE_HEAD_REG(base), command);
    outb(SEC_CNT_REG(base), 0);
    outb(SEC_NUM_REG(base), 0);
    outb(CYL_LOW_REG(base), 0);
    outb(CYL_HIGH_REG(base), 0);

    // Send identify command
    outb(CMD_REG(base), CMD_IDENTIFY);

    // Read status port
    status = inb(STAT_REG(base));
    if (status == 0) {
        printb("ata_probe(): Drive does not exist\n");
        return NULL;
    }

    // Drive exists, poll status port until bit 7 clears
    do {
        status = inb(STAT_REG(base));
    } while ((status & STAT_BSY) != 0);

    // Check LBAmid and LBAhi to see if they are non zero
    // If so, the drive is not ATA
    if (inb(CYL_LOW_REG(base)) != 0 || inb(CYL_HIGH_REG(base)) != 0) {
        printb("ata_probe(): Drive is not ATA\n");
        return NULL;
    }

    // Continue polling status until bit 3 or bit 0 sets
    do {
        status = inb(STAT_REG(base));
    } while ((status & STAT_DRQ) != 0 && (status & STAT_ERR) != 0);

    // Check if ERR is clear
    if (inb(ERROR_REG(base)) != 0) {
        printb("ata_probe(): Error after IDENTIFY\n");
        return NULL;
    }
    
    // Data is ready to be read from the data port
    // Read 256 16 bit values
    for (i = 0; i < 256; i++) {
        data[i] = inw(DATA_REG(base));
    }

    // Ensure drive supports LBA48
    if ((data[83] & 0x400) == 0) {
        printb("ata_probe(): Device doesn't support LBA48 mode\n");
        return NULL;
    }

    sectors |= (uint64_t) data[100] << 0;
    sectors |= (uint64_t) data[101] << 16;
    sectors |= (uint64_t) data[102] << 32;
    sectors |= (uint64_t) data[103] << 48;

    printb("Identified ATA drive (%s)\n", name);

    // Ready to allocate ATA dev struct
    ata_dev = (ATA_block_dev_t *)kmalloc(sizeof(ATA_block_dev_t));

    ata_dev->ata_base = base;
    ata_dev->slave = slave;
    ata_dev->dev.tot_len = sectors;
    ata_dev->dev.read_block = ATA_read_block;
    ata_dev->dev.blk_size = 512;
    ata_dev->dev.type = MASS_STORAGE;
    ata_dev->dev.name = name;
    ata_dev->dev.next = NULL;

    // Register ATA device with block driver
    BLK_register(&ata_dev->dev);

    return ata_dev;
}