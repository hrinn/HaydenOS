#include "block_dev.h"
#include "ioport.h"
#include "printk.h"
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

// Control PIO
#define ALT_STAT_REG 0x3F7              // R
#define DEV_CTRL_REG 0x3F7              // W
#define DEV_ADDR_REG 0x3F8              // R

// Register bits

// Values
#define FLOATING_BUS 0xFF

// Commands
#define CMD_SELECT_MASTER 0xA0
#define CMD_SELECT_SLAVE 0xB0

// IRQ
#define PRIMARY_INT_LINE 14
#define SECONDARY_INT_LINE 15
#define PRIMARY_IRQ PRIMARY_INT_LINE + 32
#define SECONDARY_IRQ SECONDARY_INT_LINE + 32

// Ensures specified controller is present
// Returns a struct with its information
ata_block_dev_t *ATA_probe(uint16_t base, uint16_t master, 
    uint8_t slave, const char *name, uint8_t irq) 
{
    uint8_t status, command;

    // Turn off interrupts

    // Floating bus detection
    status = inb(STAT_REG(base));
    if (status == FLOATING_BUS) {
        printk("ata_probe(): No disks on IDE bus\n");
        return NULL;
    }

    // IDENTIFY command
    command = (slave) ? CMD_SELECT_SLAVE : CMD_SELECT_MASTER;
    outb(DRIVE_HEAD_REG(base), command);
    outb(SEC_CNT_REG(base), 0);
    outb(SEC_NUM_REG(base), 0);
    outb(CYL_LOW_REG(base), 0);
    outb(CYL_HIGH_REG(base), 0);

    return NULL;
}