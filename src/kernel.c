#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"

void print_welcome() {
    printk("Welcome to ");
    VGA_set_fg_color(VGA_LIGHT_GREEN);
    printk("HaydenOS\n");
    VGA_set_fg_color(VGA_WHITE);
}

void page_fault() {
    uint32_t *bad_addr = (uint32_t *)0xDEADBEEF;
    *bad_addr = 0;
}

void kmain() {
    GDB_PAUSE; // set gdbp=1
    
    GDT_remap();
    TSS_init();
    VGA_clear();
    IRQ_init();
    SER_init();
    keyboard_init();

    print_welcome();    

    while (1) asm("hlt");
}