#include "printk.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    printk("Welcome to HaydenOS\n");
    printk("Kernel begins execution at %p\n", kmain);
    printk("%x\n", 0xdeadbeef);

    while (1) {
        asm("hlt");
    }
}