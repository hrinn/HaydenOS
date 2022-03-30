#include "printk.h"
#include "keyboard.h"
#include "vga.h"

void print_welcome() {
    printk("Welcome to ");
    VGA_set_fg_color(LIGHT_GREEN);
    printk("HaydenOS\n");
    VGA_set_fg_color(WHITE);
}   

void kmain() {
    VGA_clear();

    print_welcome();
    init_ps2_controller();

    while (1) {
        asm("hlt");
    }
}