#include "vga.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#define VGA_ADDR 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

uint16_t cursor;
uint16_t *vga = (uint16_t *)VGA_ADDR;
uint8_t fg_color = WHITE;
uint8_t bg_color = BLACK;

void VGA_clear() {
    memset(vga, 0, VGA_WIDTH * VGA_HEIGHT * 2);
    cursor = 0;
}

void VGA_display_char(char c) {
    if (c == '\n') {
        cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    } else {
        vga[cursor++] = (bg_color << 12) | (fg_color << 8) | c;
    }
}

void VGA_display_str(const char *s) {
    if (s == NULL) return;

    while(*s) {
        VGA_display_char(*s);
        s++;
    }
}

void VGA_display_strn(const char *s, int n) {
    int i;
    if (s == NULL) return;

    for (i = 0; s[i] && i < n; i++) {
        VGA_display_char(s[i]);
    }
}

void VGA_set_bg_color(char bg) {
    bg_color = bg;
}

// Foreground color and intensity bit
void VGA_set_fg_color(char fg) {
    fg_color = fg;
}