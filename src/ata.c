#include "ata.h"
#include "ioport.h"
#include "printk.h"
#include "irq.h"
#include "kmalloc.h"
#include "memdef.h"
#include <stddef.h>
#include <stdbool.h>

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

// Values
#define FLOATING_BUS 0xFF

// Commands
#define CMD_SELECT_MASTER 0xA0
#define CMD_SELECT_SLAVE 0xB0
#define CMD_IDENTIFY 0xEC
#define CMD_READ_SECTORS_EXT 0x24

// Inserts ATA request at end of device's request queue
void append_ATA_request(ATA_block_dev_t *dev, ATA_request_t *request) {
    if (dev->req_head == NULL) {
        dev->req_head = request;
        dev->req_tail = request;
    } else {
        // Insert at end of list
        dev->req_tail->next = request;
        dev->req_tail = request;
    }
}

// Fetches ATA request from beginning of device's request queue
ATA_request_t *pop_ATA_request(ATA_block_dev_t *dev) {
    ATA_request_t *result = dev->req_head;

    if (result == NULL) return NULL;

    if (result == dev->req_tail) {
        dev->req_tail = NULL;
        dev->req_head = NULL;
    } else {
        dev->req_head = dev->req_head->next;
    }
    
    return result;
}

bool is_ATA_queue_empty(ATA_block_dev_t *dev) {
    return dev->req_head == NULL;
}

bool is_ATA_request_queued(ATA_block_dev_t *dev, ATA_request_t *req) {
    ATA_request_t *current = dev->req_head;

    while (current != NULL) {
        if (current == req) {
            return true;
        }
        current = current->next;
    }

    return false;
}

void ATA_isr(uint8_t irq, uint32_t error_code, void *arg) {
    ATA_block_dev_t *ata_dev = (ATA_block_dev_t *)arg;
    ATA_request_t *request = pop_ATA_request(ata_dev);
    uint16_t *dst;
    int i;

    if (request != NULL) {
        dst = (uint16_t *)request->dst;

        // Read 256 16 bit values from data port, into request's dst
        for (i = 0; i < 256; i++) {
            dst[i] = inw(DATA_REG(ata_dev->ata_base));
        }

        // Unblock head of ATA blocked queue
        PROC_unblock_head(&ata_dev->blocked_queue);
    }

    // Read status register to clear interrupt flag
    inb(STAT_REG(ata_dev->ata_base));
    IRQ_end_of_interrupt(irq);
}

int ATA_read_block(block_dev_t *dev, uint64_t blk_num, void *dst) {
    ATA_block_dev_t *ata_dev = (ATA_block_dev_t *)dev;
    
    uint8_t drive_head;
    uint8_t *lba_n = (uint8_t *)&blk_num;

    // Create an ATA request for the operation
    ATA_request_t read_request;
    read_request.dst = dst;
    append_ATA_request(ata_dev, &read_request);

    // Select master / slave
    drive_head = (ata_dev->slave) ? 0x50: 0x40;
    outb(DRIVE_HEAD_REG(ata_dev->ata_base), drive_head);

    // Send address
    outb(SEC_CNT_REG(ata_dev->ata_base), 0); 
    outb(SEC_NUM_REG(ata_dev->ata_base), lba_n[3]);
    outb(CYL_LOW_REG(ata_dev->ata_base), lba_n[4]);
    outb(CYL_HIGH_REG(ata_dev->ata_base), lba_n[5]);
    outb(SEC_CNT_REG(ata_dev->ata_base), 1);
    outb(SEC_NUM_REG(ata_dev->ata_base), lba_n[0]);
    outb(CYL_LOW_REG(ata_dev->ata_base), lba_n[1]);
    outb(CYL_HIGH_REG(ata_dev->ata_base), lba_n[2]);

    // Poll for not busy
    // do {
    //     status = inb(ALT_STAT_REG(ata_dev->ata_base));
    // } while (status & STAT_BSY);

    // Send read sectors command
    outb(CMD_REG(ata_dev->ata_base), CMD_READ_SECTORS_EXT);

    // Block until data is ready
    // wait_event_interruptable(&ata_dev->blocked_queue, is_ATA_request_queued(ata_dev, &read_request));
    wait_event_interruptable(&ata_dev->blocked_queue, is_ATA_request_queued(ata_dev, &read_request));

    return 1;
}

// Ensures specified controller is present
// Returns a pointer to a struct with its information
ATA_block_dev_t *ATA_probe(uint16_t base, uint16_t master, 
    uint8_t slave, const char *name, uint8_t irq) 
{
    uint8_t status, command;
    uint16_t data[256];
    uint64_t sectors = 0;
    ATA_block_dev_t *ata_dev;
    int i;

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

    sectors |= (uint64_t) data[100] << 0;
    sectors |= (uint64_t) data[101] << 16;
    sectors |= (uint64_t) data[102] << 32;
    sectors |= (uint64_t) data[103] << 48;

    // Ready to allocate ATA dev struct
    ata_dev = (ATA_block_dev_t *)kmalloc(sizeof(ATA_block_dev_t));

    ata_dev->ata_base = base;
    ata_dev->ata_master = master;
    ata_dev->slave = slave;
    ata_dev->irq = irq;
    ata_dev->req_head = NULL;
    ata_dev->req_tail = NULL;
    ata_dev->dev.tot_len = sectors; // Might need to multiply by 512
    ata_dev->dev.read_block = (read_block_f)(((uint64_t)ATA_read_block) + KERNEL_TEXT_START);
    ata_dev->dev.blk_size = 512;
    ata_dev->dev.type = MASS_STORAGE;
    ata_dev->dev.name = name;
    ata_dev->dev.next = NULL;

    PROC_init_queue(&ata_dev->blocked_queue);

    // Register ATA device with block driver
    BLK_register(&ata_dev->dev);

    // Register interrupt handler
    IRQ_set_handler(irq, ATA_isr, ata_dev);
    IRQ_clear_mask(irq);

    // Set nIEN to 0 to receive interrupts
    outb(DEV_CTRL_REG(base), 0);

    return ata_dev;
}