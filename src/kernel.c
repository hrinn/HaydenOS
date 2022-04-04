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

    printk("Int: %d\n", -1);
    printk("Short int: %hd\n", (short int)-1);
    printk("Long int: %ld\n", (long int)-1);
    printk("Quad int: %qd\n", (long long int)-1);
    printk("Unsigned int %u\n", -1);
    printk("Unsigned short int: %hu\n", (unsigned short int)-1);
    printk("Unsigned long int: %lu\n", (unsigned long int)-1);
    printk("Unsigned quad int: %qu\n", (unsigned long long int)-1);
    printk("Hex: %x\n", -1);
    printk("Short hex: %hx\n", (unsigned short int)-1);
    printk("Long hex: %lx\n", (unsigned long int)-1);
    printk("Quad hex: %qx\n", (unsigned long long int)-1);

    int p = 0;
    printk("Pointer: %p\n", (void *)&p);
    printk("Char: %c\n", 'c');
    printk("String: %s\n", "a string");
    printk("100%%\n");
    
    halt();
}