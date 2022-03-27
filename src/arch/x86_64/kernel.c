#include "printk.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    printk("Welcome to HaydenOS\nColors:\n");


    for (int color = 0x0; color <= 0xF; color++) {
        VGA_set_fg_color(color);
        if (color & 0x8) {
            VGA_set_bg_color(BLACK);
        } else {
            VGA_set_bg_color(WHITE);
        }
        printk("%x - 0x%X\n", color, 0xb000dead);
    }

    while (1) {
        asm("hlt");
    }
}