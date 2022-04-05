#include "printk.h"
// #include "keyboard.h"
#include "vga.h"
#include "irq.h"

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

    IRQ_init();

    // asm volatile ("int3");

    printk("Return to kmain!\n");
    
    halt();
}