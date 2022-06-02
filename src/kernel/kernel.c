#include "printk.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "proc.h"
#include "sys_call.h"
#include "part.h"
#include "fat.h"
#include <stddef.h>
#include "keyboard.h"
#include "vfs.h"
#include "snakes.h"
#include "string.h"
#include "kmalloc.h"
#include "sys_call_ints.h"
#include "elf.h"

void kmain_vspace(void);
void kmain_thread(void *);

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
    printk("\nExecuting in kmain\n");

    // Initialize memory management
    parse_multiboot_tags(multiboot_tags);
    setup_pml4(stack_addresses);

    // Remap TSS stack addresses
    TSS_remap(stack_addresses + 1, 3);

    // Switch execution to kernel space
    asm ( "movq %0, %%rsp" : : "r"(stack_addresses[0]));
    VSPACE(kmain_vspace)();
}

void kmain_vspace() {
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);

    init_sys_calls();
    PROC_init();
    PROC_create_kthread(VSPACE(kmain_thread), NULL);

    while (1) {
        CLI;
        PROC_run();
        STI;
        asm ("hlt");
    }
}

void launch_user_process(inode_t *root, char *binary_path) {
    virtual_addr_t prog_start;
    prog_start = ELF_mmap_binary(root, binary_path);

    if (prog_start != 0) {
        printk("\nExecuting user program (%p)\n", (void *)prog_start);
        PROC_create_uthread((uproc_t)prog_start);
    }
}

void kmain_thread(void *arg) {
    part_block_dev_t *partitions[4];
    ATA_block_dev_t *drive;
    superblock_t *superblock;

    printb("\nExecuting in kthread\n");

    drive = ATA_probe(PRIMARY_BASE, 0, "sda", PRIMARY_IRQ);
    parse_MBR(drive, partitions);
    FS_register(VSPACE(FAT_detect));
    superblock = FS_probe((block_dev_t *)partitions[0]);

    KBD_init();

    // Launch user process
    launch_user_process(superblock->root_inode, "/bin/test.bin");
}