#include "printk.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "proc.h"
#include "block_dev.h"
#include "sys_call.h"
#include <stddef.h>

#define HALT_LOOP while(1) asm("hlt")

void kmain_vspace(void);
void kmain_thread(void *);
void keyboard_printer(void *);

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
    (kmain_vspace + KERNEL_TEXT_START)();
}
#pragma GCC diagnostic pop

void kmain_vspace() {
    // Apply DATA REL offsets
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);

    init_sys_calls();
    PROC_init();
    PROC_create_kthread(kmain_thread, NULL);

    while (1) {
        PROC_run();
        asm ("hlt");
    }
}

void kmain_thread(void *arg) {
    ata_block_dev_t *ata_dev;
    printk("Executing in kthread\n");

    ata_dev = ATA_probe(PRIMARY_BASE, 0, 0, "ATA Drive", PRIMARY_IRQ);
    ATA_read_block(ata_dev, 32, NULL);
}