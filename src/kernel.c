#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "kmalloc.h"

void kmain_stage2(void);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpointer-arith"
void kmain(struct multiboot_info *multiboot_tags) {
    virtual_addr_t stack_addresses[4];
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
    setup_pml4(stack_addresses);

    // Remap TSS stack addresses
    TSS_remap(stack_addresses + 1, 3);

    // Switch execution to kernel space
    asm ( "movq %0, %%rsp" : : "r"(stack_addresses[0]));
    (kmain_stage2 + KERNEL_TEXT_START)();
}
#pragma GCC diagnostic pop

void test_kmalloc() {
    int n = 0;

    void *addresses[100];
    void **adp = addresses;

    printk("TEST: Allocating block sizes\n");

    for (n = 32; n <= 2048; n *= 2) {
        *adp = kmalloc(n - 16);
        printk("%d + 16: %p\n", n - 16, *adp);
        adp++;
    }

    printk("TEST: Allocating just above block size\n");

    for (n = 32; n <= 2048; n *= 2) {
        *adp = kmalloc(n);
        printk("%d: %p\n", n, *adp);
        adp++;
    }

    printk("TEST: Freeing all allocated blocks\n");
    while (adp > addresses) {
        kfree(*--adp);
    }

    printk("TEST: Allocate 100 64B blocks\n");
    for (n = 0; n < 100; n++) {
        addresses[n] = kmalloc(48);
    }

    printk("TEST: Free 100 64B blocks\n");
    for (n = 0; n < 100; n++) {
        kfree(addresses[n]);
    }

    printk("TEST: Allocate blocks until memory fills\n");
    while (1) {
        kmalloc(4096);
    }
}

void kmain_stage2() {
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);
    printk("Executing in kernel space\n");

    test_kmalloc();

    while (1) asm("hlt");
}