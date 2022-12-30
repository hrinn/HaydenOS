#include "pci.h"
#include "ioport.h"
#include <stdint-gcc.h>
#include "printk.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

#define PCI_BAD_VENDOR 0xFFFF

// Reads 16 bits from the PCI configuration space through I/O ports
uint16_t PCI_config_readw(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;

    uint32_t address = (uint32_t)(
        (lbus << 16) | 
        (lslot << 11) | 
        (lfunc << 8) | 
        (offset & 0xFC) | 
        ((uint32_t)0x80000000)
    );

    outl(CONFIG_ADDRESS, address);

    return (uint16_t)((inl(CONFIG_DATA) >> ((offset & 2) * 8)) & 0xFFFF);
}

uint16_t get_vendor(uint8_t bus, uint8_t slot, uint8_t func) {
    return PCI_config_readw(bus, slot, func, 0);
}

uint8_t get_class(uint8_t bus, uint8_t slot, uint8_t func) {
    return PCI_config_readw(bus, slot, func, 0xA) >> 8;
}

uint8_t get_subclass(uint8_t bus, uint8_t slot, uint8_t func) {
    return PCI_config_readw(bus, slot, func, 0xA) & 0xFF;
}

uint8_t get_header_type(uint8_t bus, uint8_t slot, uint8_t func) {
    return PCI_config_readw(bus, slot, func, 0xE) & 0xFF;
}

// Returns 1 if present, -1 if absent
int PCI_check_func(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor;

    if ((vendor = get_vendor(bus, slot, func)) == PCI_BAD_VENDOR) {
        return -1;
    }

    printb("PCI (%d, %d, %d) - vendor: 0x%x, class: 0x%x, subclass: 0x%x\n",
        bus, slot, func,
        get_vendor(bus, slot, func),
        get_class(bus, slot, func),
        get_subclass(bus, slot, func)
    );

    return 1;
}

void PCI_check_device(uint8_t bus, uint8_t slot) {
    uint8_t func = 0;

    if (PCI_check_func(bus, slot, func) < 0) {
        return;
    }

    if ((get_header_type(bus, slot, func) & 0x80) != 0) {
        // Multi function device; check remaining functions
        for (func = 1; func < 8; func++) {
            if (PCI_check_func(bus, slot, func) < 0) {
                break;
            }
        }
    }
}

void PCI_enumerate() {
    uint16_t bus;
    uint8_t slot;

    printb("Enumerating PCI\n");

    for (bus = 0; bus < 256; bus++) {
        for (slot = 0; slot < 32; slot++) {
            PCI_check_device(bus, slot);
        }
    }
}