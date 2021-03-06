#ifndef IOPORT_H
#define IOPORT_H

#include <stdint-gcc.h>

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(port));
    return ret;
}

static inline uint16_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0"
                   : "=a"(ret)
                   : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t byte) {
    asm volatile ("outb %0, %1"
                    : /* No output registers */
                    : "a"(byte) , "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t byte) {
    asm volatile ("outw %0, %1"
                    : /* No output registers */
                    : "a"(byte) , "Nd"(port));
}

static inline void outl(uint16_t port, uint32_t byte) {
    asm volatile ("outl %0, %1"
                    : /* No output registers */
                    : "a"(byte) , "Nd"(port));
}

static inline void io_wait() {
    outb(0x80, 0);
}

#endif