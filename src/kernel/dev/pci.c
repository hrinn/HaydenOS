#include "pci.h"
#include "ioport.h"
#include <stdint-gcc.h>
#include "printk.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

struct PCI_header {
    uint16_t device_id;
    uint16_t vendor_id;
    uint16_t status;
    uint16_t command;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint8_t bist;
    uint8_t header_type;
    uint8_t latency_timer;
    uint8_t cache_line_size;
};

// Reads 16 bits from the PCI configuration space through I/O ports
uint16_t PCI_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)(
        (bus << 16) | 
        (slot << 11) | 
        (func << 8) | 
        (offset & 0xFC) | 
        ((uint32_t)0x80000000)
    );

    outl(CONFIG_ADDRESS, address);

    return (uint16_t)((inl(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

void PCI_enumerate() {
    printk("0x%x\n", PCI_config_read_word(0, 0, 0, 0));
}