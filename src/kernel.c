#include "printk.h"
#include "keyboard.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "proc.h"
#include <stddef.h>
#include "scheduler.h"

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

void thread(void *name) {
    int i;
    for (i = 0; i < 3; i++) {
        printk("thread %s: %d\n", (char *)name, i);
        yield();
    }
    kexit();
}

void kmain_stage2() {
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);
    printk("Executing in kernel space\n");

    PROC_init();

    PROC_create_kthread(thread, "A");
    PROC_create_kthread(thread, "B");
    PROC_create_kthread(thread, "C");

    PROC_run();
    printk("Back to kmain!\n");

    while (1) asm ("hlt");
}