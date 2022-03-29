#include "printk.h"
#include "keyboard.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    printk("Welcome to ");
    VGA_set_fg_color(LIGHT_GREEN);
    printk("HaydenOS\n");
    VGA_set_fg_color(WHITE);

    while (1) {
        asm("hlt");
    }
}