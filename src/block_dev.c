#include "block_dev.h"
#include "ioport.h"
#include "printk.h"
#include "irq.h"
#include "kmalloc.h"
#include <stddef.h>

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
#define ATA_STAT_REG(base) base + 0x206
#define DEV_CTRL_REG(base) base + 0x206
#define DEV_ADDR_REG(base) base + 0x207

// Register bits
#define DEV_CTRL_NIEN 0x2

// Values
#define FLOATING_BUS 0xFF

// Commands
#define CMD_SELECT_MASTER 0xA0
#define CMD_SELECT_SLAVE 0xB0
#define CMD_IDENTIFY 0xEC

#define PRIMARY_INT_LINE 14

void ATA_isr(uint8_t irq, uint32_t error_code, void *arg) {
    ata_block_dev_t *ata_dev = (ata_block_dev_t *)arg;

    printk("ATA Isr! %s\n", ata_dev->dev.name);

    IRQ_end_of_interrupt(irq - 32);
}

int ATA_read_block(block_dev_t *dev, uint64_t blk_num, void *dst) {
    
}

// Ensures specified controller is present
// Returns a pointer to a struct with its information
// This struct must be freed
ata_block_dev_t *ATA_probe(uint16_t base, uint16_t master, 
    uint8_t slave, const char *name, uint8_t irq) 
{
    uint8_t status, command;
    uint16_t data[256];
    uint64_t sectors = 0;
    ata_block_dev_t *ata_dev;
    int i;

    // Turn off interrupts
    outb(DEV_CTRL_REG(base), DEV_CTRL_NIEN);

    // Floating bus detection
    status = inb(STAT_REG(base));
    if (status == FLOATING_BUS) {
        printk("ata_probe(): No disks on IDE bus\n");
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
        printk("ata_probe(): Drive does not exist\n");
        return NULL;
    }

    // Drive exists, poll status port until bit 7 clears
    do {
        status = inb(STAT_REG(base));
    } while ((status & 0x80) != 0);

    // Check LBAmid and LBAhi to see if they are non zero
    // If so, the drive is not ATA
    if (inb(CYL_LOW_REG(base)) != 0 || inb(CYL_HIGH_REG(base)) != 0) {
        printk("ata_probe(): Drive is not ATA\n");
        return NULL;
    }

    // Continue polling status until bit 3 or bit 0 sets
    do {
        status = inb(STAT_REG(base));
    } while ((status & 0x8) != 0 && (status & 0x1) != 0);

    // Check if ERR is clear
    if (inb(ERROR_REG(base)) != 0) {
        printk("ata_probe(): Error after IDENTIFY\n");
        return NULL;
    }
    
    // Data is ready to be read from the data port
    // Read 256 16 bit values
    for (i = 0; i < 256; i++) {
        data[i] = inw(DATA_REG(base));
    }

    // Ensure drive supports LBA48
    if ((data[83] & 0x400) == 0) {
        printk("ata_probe(): Device doesn't support LBA48 mode\n");
        return NULL;
    }

    // Determine API mode

    sectors |= (uint64_t) data[100] << 0;
    sectors |= (uint64_t) data[101] << 16;
    sectors |= (uint64_t) data[102] << 32;
    sectors |= (uint64_t) data[103] << 48;

    // Ready to allocate ATA dev struct
    ata_dev = (ata_block_dev_t *)kmalloc(sizeof(ata_block_dev_t));

    ata_dev->ata_base = base;
    ata_dev->ata_master = master;
    ata_dev->slave = slave;
    ata_dev->irq = irq;
    ata_dev->req_head = NULL;
    ata_dev->req_tail = NULL;
    ata_dev->dev.tot_len = sectors; // Might need to multiply by 512
    ata_dev->dev.blk_size = 512;
    ata_dev->dev.type = PARTITION; // Not sure if this is right
    ata_dev->dev.name = name;
    ata_dev->dev.next = NULL;

    // Register interrupt handler
    IRQ_set_handler(irq, ATA_isr, ata_dev);
    IRQ_clear_mask(irq - 32);

    // Turn interrupts back on
    outb(DEV_CTRL_REG(base), 0);

    return ata_dev;
}