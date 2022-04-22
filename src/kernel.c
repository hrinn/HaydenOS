#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "page_table.h"
#include "multiboot.h"

#define TESTN 6

void page_fault() {
    uint32_t *bad_addr = (uint32_t *)0xDEADBEEF;
    *bad_addr = 0;
}

void kmain(struct multiboot_info *multiboot_tags) {
    GDB_PAUSE; // set gdbp=1
    
    // Remap GDT and initialize TSS
    GDT_remap();
    TSS_init();

    // Enable interrupts and serial driver
    IRQ_init();
    SER_init();

    VGA_clear();
    printk("Hello, world!\n");

    // Initialize memory management
    parse_multiboot_tags(multiboot_tags);
    setup_pml4();

    printk("Am I alive?\n");

    // Null pages shouldn't work now
    uint64_t *null = 0;
    printk("%ld\n", *null);

    while (1) asm("hlt");
}