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

    int i = 54321;

    // printk("short int: %hd test\n", (short int)i);
    // printk("long int: %ld\n", (long int)i);
    // printk("long long int: %qd\n", (long long int)i);
    printk("int: %d test\n", i);
    printk("hex: %x\n", i);
    printk("100%%\n");
    printk("char: %c\n", 'c');
    printk("pointer: %p\n", (void *)&i);
    printk("string: %s\n", "test");

    // Test printd

    // Print user input
    // while (1) {
    //     printk("%c", poll_keyboard());
    // }
    
    halt();
}