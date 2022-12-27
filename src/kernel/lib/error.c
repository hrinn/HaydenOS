#include "error.h"
#include "printk.h"

void panic(const char *msg) {
    printk("Panic!\n%s\nHalting...\n", msg);

    // TODO: Dump debug information

    while (1) asm ("hlt");
}