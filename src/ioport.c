#include "ioport.h"

uint8_t inb(uint16_t port) {
    uint8_t data;
    asm("inb %0, %1"
        : "=r"(data)
        : "r"(port)
        : );
    return data;
}