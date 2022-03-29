#include "printk.h"
#include "keyboard.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    printk("Welcome to HaydenOS\n");

    if (init_ps2_controller() != 1) {
        printk("Error initializing ps2 controller");
    } else {
        printk("Initialized PS/2 controller");
    }

    while (1) {
        asm("hlt");
    }
}