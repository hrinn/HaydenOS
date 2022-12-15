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
extern void call_user(uint64_t user_text, uint64_t user_stack);

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
    // TSS_remap(stack_addresses + 1, 3);

    // Switch execution to kernel space
    // asm ( "movq %0, %%rsp" : : "r"(stack_addresses[0]));
    kmain_vspace();
}

void kmain_vspace() {
    cleanup_old_virtual_space();

    init_sys_calls();
    PROC_init();
    PROC_create_kthread(kmain_thread, NULL);

    while (1) {
        CLI;
        PROC_run();
        STI;
        asm ("hlt");
    }
}

void setup_userspace(inode_t *root, char *binary_path) {
    virtual_addr_t prog_start;
    prog_start = ELF_mmap_binary(root, binary_path);

    if (prog_start == 0) return;

    // Setup userspace stack
    user_allocate_range(USER_STACK_START, PAGE_SIZE * 10);

    // Setup user -> kernel stack
    TSS_set_rsp(allocate_thread_stack(), 0);

    printk("Jumping to user space... (%p)\n", (void *)prog_start);
    call_user(prog_start, USER_STACK_START + PAGE_SIZE * 10);
}

void kmain_thread(void *arg) {
    part_block_dev_t *partitions[4];
    ATA_block_dev_t *drive;
    superblock_t *superblock;

    printb("\nExecuting in kthread\n");

    drive = ATA_probe(PRIMARY_BASE, 0, "sda", PRIMARY_IRQ);
    parse_MBR(drive, partitions);
    FS_register(FAT_detect);
    superblock = FS_probe((block_dev_t *)partitions[0]);

    KBD_init();

    setup_userspace(superblock->root_inode, "/bin/init.bin");
}