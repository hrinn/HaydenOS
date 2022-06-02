#include "sys_call_ints.h"
#include "stdint-gcc.h"

void main() {
    // Test memory protection
    // uint64_t *pml4 = (uint64_t *)0x1000;
    // pml4[2] = 0;

    // Test privileged instructions
    uint16_t port = 0x64; // PS2 Command Port
    uint8_t data = 0xAD;  // Disable P1
    asm ("outb %0, %1" : : "a"(data), "Nd"(port));

    while (1) {
        putc(getc());
    }
}