#ifndef IOPORT_H
#define IOPORT_H

#include <stdint.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port));
    return ret;
}

static inline void outb(uint8_t byte, uint16_t port) {
    asm volatile ("outb %0, %1"
                    : /* No output registers */
                    : "a"(byte) , "Nd"(port));
}

// void outb(uint8_t byte, uint16_t port);
// uint16_t inw(uint16_t port);
// void outw(uint16_t word, uint16_t port);
// uint32_t inl(uint16_t port);
// void outl(uint32_t word, uint16_t port);

#endif