#include "vga.h"
#include <stdint-gcc.h>
#include <stddef.h>
#include "string.h"
#include "irq.h"

#define VGA_ADDR 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TAB_WIDTH 4
#define LINE(x) x - (x % VGA_WIDTH)

static uint16_t cursor;
static uint16_t *vga = (uint16_t *)VGA_ADDR;
static uint8_t fg_color = VGA_WHITE;
static uint8_t bg_color = VGA_BLACK;

void VGA_clear() {
    uint16_t int_en = check_int();
    if (int_en) cli();

    memset(vga, 0, VGA_WIDTH * VGA_HEIGHT * 2);
    cursor = 0;

    if (int_en) sti();
}

void VGA_paint() {
    int i;
    uint16_t color;
    uint16_t int_en = check_int();
    if (int_en) cli();

    color = (bg_color << 12) | (fg_color << 8);
    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = color;
    }
    cursor = 0;

    if (int_en) sti();
}

void scroll() {
    memcpy(vga, vga + VGA_WIDTH, ((VGA_HEIGHT - 1) * VGA_WIDTH) * 2);
    memset(vga + ((VGA_HEIGHT - 1) * VGA_WIDTH), 0, VGA_WIDTH * 2);
    cursor = (VGA_HEIGHT - 1) * VGA_WIDTH;
} 

void VGA_display_char(char c) {
    uint16_t int_en = check_int();
    if (int_en) cli();

    switch (c) {
        case '\n':
            cursor = LINE(cursor) + VGA_WIDTH;
            break;
        case '\b':
            if (cursor == 0) break;
            vga[--cursor] = (bg_color << 12) | (fg_color << 8);
            break;
        case '\t':
            cursor += TAB_WIDTH;
            break;
        default:
            vga[cursor++] = (bg_color << 12) | (fg_color << 8) | c;
            break;
    }

    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        scroll();
    }

    if (int_en) sti();
}

void VGA_display_str(const char *s, int len) {
    int i;
    if (s == NULL) return;

    for (i = 0; s[i] && i < len; i++) {
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

int VGA_row_count(void) {
    return VGA_WIDTH;
}

int VGA_col_count(void) {
    return VGA_HEIGHT;
}

void VGA_display_attr_char(int x, int y, char c, int fg, int bg) {
    unsigned short out = (bg << 12) | (fg << 8) | c;
    vga[x * y] = out;
}