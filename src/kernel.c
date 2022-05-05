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
#include "snakes.h"
#include "sys_call.h"

#define HALT_LOOP while(1) asm("hlt")

void kmain_2(void);
void kmain_3(void *);
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
    (kmain_2 + KERNEL_TEXT_START)();
}
#pragma GCC diagnostic pop

void thread(void *str) {
    int i;
    for (i = 0; i < 5; i++) {
        printk("thread %s\n", (char *)str);
        yield();
    }
    kexit();
}

void kmain_2() {
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);

    init_sys_calls();
    PROC_init();
    PROC_create_kthread(kmain_3, NULL);

    while (1) {
        PROC_run();
        printk("Back to kmain!\n");
        asm ("hlt");
    }
}

void kmain_3(void *arg) {
    printk("Executing in kthread\n");

    KBD_init();
    PROC_create_kthread(keyboard_printer, NULL);
    
    while (1) {
        yield();
        asm ("hlt");
    }
}

void keyboard_printer(void *arg) {
    printk("Keyboard input:\n");
    while (1) {
        printk("%c", getc());
    }
}