#include "printk.h"
#include "keyboard.h"
#include "vga.h"

void print_welcome() {
    printk("Welcome to ");
    VGA_set_fg_color(VGA_LIGHT_GREEN);
    printk("HaydenOS\n");
    VGA_set_fg_color(VGA_WHITE);
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

    // Print user input
    while (1) {
        printk("%c", poll_keyboard());
    }
    
    halt();
}