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
#include "md5.h"

#define HALT_LOOP while(1) asm("hlt")

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
    // Apply DATA REL offsets
    apply_isr_offset(KERNEL_TEXT_START);
    cleanup_old_virtual_space();    // This also sets identity mapped region to no execute
    SER_kspace_offset(KERNEL_TEXT_START);

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

void print_file_by_path(char *path, superblock_t *superblock) {
    file_t *file;
    inode_t *inode;
    inode = FS_inode_for_path(path, superblock->root_inode);
    file = inode->open(inode);
    char buffer[513];
    int len;
    buffer[512] = 0;

    printk("\n%s:\n", path);

    while ((len = file->read(file, buffer, 512))) {
        printb("%s", buffer);
    }
    file->close(&file);
}

void print_partition(part_block_dev_t *part) {
    block_dev_t *dev = (block_dev_t *)part;
    char buffer[513];
    int i = 0;
    buffer[512] = 0;
    while (dev->read_block(dev, i++, buffer) != -1) {
        printb("%s", buffer);
    }
}

void keyboard_reader() {
    KBD_init();
    while(1) {
        printk("%c", getc());
    }
}

void md5_file(char *path, superblock_t *superblock) {
    file_t *file;
    inode_t *inode;
    inode = FS_inode_for_path(path, superblock->root_inode);
    file = inode->open(inode);

    MD5_CTX context;
    unsigned char buffer[1024], digest[16];
    int len;
    printk("\nMD5 of %s:\n", path);

    MD5Init(&context);
    while ((len = file->read(file, (char *)buffer, 1024))) {
        MD5Update(&context, buffer, len);
    }
    MD5Final(digest, &context);

    file->close(&file);
    MDPrint(digest);
}

void md5_test() {
    char *message = "This is a shorter file for testing the filesystem\n";
    MD5_CTX context;
    unsigned char digest[16];
    MD5Init(&context);
    MD5Update(&context, message, strlen(message));
    MD5Final(digest, &context);
    MDPrint(digest);
}

void kmain_thread(void *arg) {
    part_block_dev_t *partitions[4];
    ATA_block_dev_t *drive;
    superblock_t *superblock;

    printb("\nExecuting in kthread\n");

    drive = ATA_probe(PRIMARY_BASE, 0, "sda", PRIMARY_IRQ);
    parse_MBR(drive, partitions);
    // print_partition(partitions[0]);

    FS_register(VSPACE(FAT_detect));
    superblock = FS_probe((block_dev_t *)partitions[0]);

    print_filesystem(superblock);
    md5_file("/test/short.txt", superblock);
    md5_file("/test/heart-of-darkness.txt", superblock);
    md5_file("/test/war-and-peace.txt", superblock);
}