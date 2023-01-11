#include "pci.h"
#include "ioport.h"
#include <stdint-gcc.h>
#include "printk.h"

#define CONFIG_ADDRESS 0xCF8
#define CONFIG_DATA 0xCFC

#define PCI_BAD_VENDOR 0xFFFF

#define CLASS_MASS_STORAGE 1
#define SUBCLASS_IDE_CONTROLLER 1

struct PCI_header_common {
    uint16_t vendor, device;
    uint16_t cmd, status;
    uint8_t rev, iface, subclass, class;
    uint8_t cacheline_size, latency_timer, hdr_type, bist;
};

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

int PCI_read_config_space(uint8_t bus, uint8_t slot, uint8_t func, struct PCI_header_common *header) {
    uint16_t *header_ptr = (uint16_t *)header;
    uint8_t i;

    header->vendor = get_vendor(bus, slot, func);

    if (header->vendor == PCI_BAD_VENDOR) {
        return -1;
    }

    for (i = 1; i < sizeof(struct PCI_header_common); i += 2) {
        *header_ptr++ = PCI_config_readw(bus, slot, func, i);
    }

    return 1;
}

// Returns 1 if present, -1 if absent
int PCI_check_func(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor;
    uint8_t class;
    uint8_t subclass;

    if ((vendor = get_vendor(bus, slot, func)) == PCI_BAD_VENDOR) {
        return -1;
    }

    class = get_class(bus, slot, func);
    subclass = get_subclass(bus, slot, func);

    if (class == CLASS_MASS_STORAGE && subclass == SUBCLASS_IDE_CONTROLLER) {
        printb("Identified IDE Controller at PCI (%d, %d, %d)\n", bus, slot, func);
    }

    return 1;
}

void PCI_check_device(uint8_t bus, uint8_t slot) {
    uint8_t func;

    if (PCI_check_func(bus, slot, 0) < 0) {
        return;
    }

    if ((get_header_type(bus, slot, 0) & 0x80) != 0) {
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

    printb("Enumerating PCI bus\n");

    for (bus = 0; bus < 256; bus++) {
        for (slot = 0; slot < 32; slot++) {
            PCI_check_device(bus, slot);
        }
    }
}