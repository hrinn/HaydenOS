#include "printk.h"
#include "keyboard.h"
#include "vga.h"

void print_welcome() {
    printk("Welcome to ");
    VGA_set_fg_color(LIGHT_GREEN);
    printk("HaydenOS\n");
    VGA_set_fg_color(WHITE);
}

void halt() {
    while (1) {
        asm("hlt");
    }
}

void kmain() {

    VGA_clear();

    print_welcome();
    
    if (init_ps2_controller() != 1) {
        printk("Failed to initialize PS/2 controller.\n");
        halt();
    }

    if (init_keyboard() != 1) {
        printk("Failed to initialize keyboard.\n");
        halt();
    }

    // Scan keycodes and print them
    while (1) {
        printk("0x%x", poll_keyboard());
    }
    
    halt();
}