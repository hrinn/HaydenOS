#include "vga.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"
#include "irq.h"

#define VGA_ADDR 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define TAB_WIDTH 4
#define LINE(x) x - (x % VGA_WIDTH)

static uint16_t cursor;
static uint16_t *vga = (uint16_t *)VGA_ADDR;
static uint8_t fg_color = VGA_WHITE;
static uint8_t bg_color = VGA_BLACK;

void set_cursor(uint16_t val) {
    cli();
    cursor = val;
    sti();
}

void VGA_clear() {
    memset(vga, 0, VGA_WIDTH * VGA_HEIGHT * 2);
    set_cursor(0);
}

void scroll() {
    memcpy(vga, vga + VGA_WIDTH, ((VGA_HEIGHT - 1) * VGA_WIDTH) * 2);
    memset(vga + ((VGA_HEIGHT - 1) * VGA_WIDTH), 0, VGA_WIDTH * 2);
    set_cursor((VGA_HEIGHT - 1) * VGA_WIDTH);
} 

void VGA_display_char(char c) {
    switch (c) {
        case '\n':
            set_cursor(LINE(cursor) + VGA_WIDTH);
            break;
        case '\b':
            if (cursor == 0) break;
            set_cursor(cursor - 1);
            vga[cursor] = (bg_color << 12) | (fg_color << 8);
            break;
        case '\t':
            set_cursor(cursor + TAB_WIDTH);
            break;
        default:
            vga[cursor] = (bg_color << 12) | (fg_color << 8) | c;
            set_cursor(cursor + 1);
            break;
    }


    if (cursor >= VGA_WIDTH * VGA_HEIGHT) {
        scroll();
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