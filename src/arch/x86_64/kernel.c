#include "printk.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    int d = -54321;

    printk("Test 1 is a long line that is longer than line 2.\n");
    printk("Test 2\n");

    while (1) {
        asm("hlt");
    }
}