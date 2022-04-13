#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"

extern void call_int(void);

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
    GDB_PAUSE; // set gdbp=1
    
    IRQ_init();

    call_int();

    keyboard_init();

    while (1) {
        printk("%c", poll_keyboard());
    }

    halt();
}