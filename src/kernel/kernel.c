#include "printk.h"
#include "vga.h"
#include "irq.h"
#include "debug.h"
#include "gdt.h"
#include "serial.h"
#include "mmu.h"
#include "proc.h"
#include "init_syscalls.h"
#include "part.h"
#include "fat.h"
#include <stddef.h>
#include "keyboard.h"
#include "vfs.h"
#include "snakes.h"
#include "string.h"
#include "kmalloc.h"
#include "syscall.h"
#include "elf.h"

void kmain_vspace(void);
void kmain_thread(void *);
extern void call_user(uint64_t user_text, uint64_t user_stack);

void kmain(struct multiboot_info *multiboot_tags) {
    virtual_addr_t stack_addresses[4];
    
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

    // TODO: Remap kernel and IST stacks
    // TSS_remap(stack_addresses + 1, 3);

    // asm ( "movq %0, %%rsp" : : "r"(stack_addresses[0]));

    free_multiboot_sections();

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

    // Probe for ATA drive on primary master bus
    if ((drive = ATA_probe(PRIMARY_BASE, 0, "sda", PRIMARY_IRQ)) == NULL) {
        printb("No ATA drive was found on primary/master");
        return;
    }

    // Parse Master Boot Record on sda
    if (parse_MBR(drive, partitions) != 1) {
        printb("Failed to parse MBR on %s\n", drive->dev.name);
        return;
    }

    // Register FAT32 filesystem in VFS
    FS_register(FAT_detect);

    // Probe sda0 for supported filesystem
    if ((superblock = FS_probe((block_dev_t *)partitions[0])) == NULL) {
        printb("Failed to find supported filesystem on %s\n", partitions[0]->ata.dev.name);
        return;
    };

    if (KBD_init() < 0) {
        printb("Failed to initialize keyboard\n");
    }

    setup_userspace(superblock->root_inode, "/bin/init.bin");
}