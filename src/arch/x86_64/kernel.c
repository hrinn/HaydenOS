#include "printk.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    int d = -54321;

    printk("d = %d, &d = %p\n", d, (void *)&d);
    printk("Putting a %s in the middle.\n", "string");
    printk("0x%X 0x%x\n", 0xDEAD, 0xBEEF);
    printk("Unsigned %u\n", -12);

    while (1) {
        asm("hlt");
    }
}