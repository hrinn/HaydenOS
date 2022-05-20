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

#define HALT_LOOP while(1) asm("hlt")

void kmain_vspace(void);
void kmain_thread(void *);
void keyboard_printer(void *);

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
    // Apply DATA REL offsets
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);

    init_sys_calls();
    PROC_init();
    PROC_create_kthread(kmain_thread, NULL);

    while (1) {
        PROC_run();
        // printk("Post PROC_run(), int status: %hx\n", check_int());
        asm ("hlt");
    }
}

int readfs_cb(const char *ent_name, inode_t *inode, void *p) {
    int n_tabs = *(int *)p, i, next_tabs = n_tabs + 1;
    char tabs[(n_tabs * 4) + 1];

    for (i = 0; i < (n_tabs * 4); i++) {
        tabs[i] = ' ';
    }
    tabs[n_tabs * 4] = 0;

    printb("%s/%s\n", tabs, ent_name);
    if (inode->st_mode & S_IFDIR) {
        inode->readdir(inode, VSPACE(readfs_cb), (void *)&next_tabs);
    }

    inode->free(&inode);

    return 1;
}

void print_filesystem(superblock_t *superblock) {
    int ntabs = 0;

    printk("\nFilesystem entries:\n");
    superblock->root_inode->readdir(superblock->root_inode, VSPACE(readfs_cb), (void *)&ntabs);
}

void print_file(file_t *file) {
    char buffer[513];
    int n;

    do {
        buffer[512] = 0;
        n = file->read(file, buffer, 512);
        printb("%s", buffer);
    } while (n == 512);
}

void print_file_by_path(char *path, superblock_t *superblock) {
    file_t *file;
    inode_t *inode;
    inode = FS_inode_for_path(path, superblock->root_inode);
    file = inode->open(inode);

    printk("%s:\n", path);
    print_file(file);
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

    print_filesystem(superblock);
    print_file_by_path("/test/short.txt", superblock);
}