#include <stdint.h>
#include <stddef.h>

#define VGA_ADDR 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define DEFAULT_COLOR 0x0F00

extern size_t strlen(const char *);
extern void *memset(void *, int, size_t);

uint16_t cursor;
uint16_t *vga = (uint16_t *)VGA_ADDR;

void VGA_clear() {
    memset(vga, 0, VGA_WIDTH * VGA_HEIGHT * 2);
    cursor = 0;
}

void VGA_display_char(char c) {
    uint16_t linepos;

    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    } else {
        vga[cursor++] = DEFAULT_COLOR | c;
    }
}

void VGA_display_str(const char *s) {
    if (s == NULL) return;

    while(*s) {
        VGA_display_char(*s);
        s++;
    }
}