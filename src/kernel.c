#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include <stdbool.h>

#define TESTN 6

void page_fault() {
    uint32_t *bad_addr = (uint32_t *)0xDEADBEEF;
    *bad_addr = 0;
}

void fill_page(void *page) {
    int i;
    uint64_t *pg = (uint64_t *)page;

    for (i = 0; i < PAGE_SIZE/8; i++) {
        pg[i] = (uint64_t)page;
    }
}

bool check_page(void *page) {
    int i;
    uint64_t *pg = (uint64_t *)page;
    
    for (i = 0; i < PAGE_SIZE / 8; i++) {
        if (pg[i] != (uint64_t)page) {
            return false;
        }
    }
    return true;
}

void print_page(void *page) {
    int i;
    uint64_t *pg = (uint64_t *)page;
    for (i = 0; i < PAGE_SIZE / 8; i++) {
        printk("%lx\n", pg[i]);
    }
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
    int i, j;
    void *page;


    for (i = 0; i < TESTN; i++) {
        for (j = 0; j < i; j++) {
            pages[j] = MMU_pf_alloc();
            printk("Allocated page %p\n", pages[j]);
        }
        for (j = 0; j < i; j++) {
            printk("Freeing page %p\n", pages[j]);
            MMU_pf_free(pages[j]);
        }
    }

    printk("Allocating and writing pages until memory fills...\n");
    while (1) {
        page = MMU_pf_alloc();
        fill_page(page);
        if (!check_page(page)) printk("Contents of page %p could not be verified\n", page);
    }
    

    while (1) asm("hlt");
}