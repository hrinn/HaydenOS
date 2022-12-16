#include "syscall.h"
#include "stdint-gcc.h"

void print(char *str, int len) {
    int i;
    for (i = 0; i < len; i++) {
        putc(str[i]);
    }
}

void main() {
    // Test memory protection
    // uint64_t *pml4 = (uint64_t *)0x1000;
    // pml4[2] = 0;

    // Test privileged instructions
    // uint16_t port = 0x64; // PS2 Command Port
    // uint8_t data = 0xAD;  // Disable P1
    // asm ("outb %0, %1" : : "a"(data), "Nd"(port));
    print("Welcome to user mode!\n", 23);

    while (1) {
        putc(getc());
    }
}