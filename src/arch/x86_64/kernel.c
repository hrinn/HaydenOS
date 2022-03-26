#include "stdio.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    printk("%X %s %d\n", 0xacab, "fuck", 12);
    printk("What the f*** is going on\n");

    while (1) {
        asm("hlt");
    }
}