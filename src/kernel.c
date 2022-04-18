#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"

#define TESTN 8

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

    void *pages[TESTN];
    int i;


    // Allocate pages
    for (i = 0; i < TESTN; i++) {
        pages[i] = MMU_pf_alloc();
        printk("Allocated page: %p\n", pages[i]);
    }

    // Free them in reverse order
    for (i = TESTN - 1; i >= 0; i--) {
        printk("Freeing page %p\n", pages[i]);
        MMU_pf_free(pages[i]);
    }

    // Allocate again
    for (i = 0; i < TESTN; i++) {
        pages[i] = MMU_pf_alloc();
        printk("Allocated page: %p\n", pages[i]);
    }

    printk("Allocating pages until memory fills...\n");
    while (1) MMU_pf_alloc();
    

    while (1) asm("hlt");
}