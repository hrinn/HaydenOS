#include "printk.h"
#include "vga.h"

void kmain() {
    VGA_clear();

    char *msg = "Welcome to HaydenOS";
    printk("%s\nStack around: %p\n", msg, msg);

    while (1) {
        asm("hlt");
    }
}