#include "elf.h"
#include "printk.h"
#include "string.h"
#include "page_table.h"

#define BITSIZE_64 2
#define LITTLE_ENDIAN 1
#define VERSION 1
#define X86_64 0
#define EXECUTABLE 2

typedef struct ELF_common_header {
    uint8_t magic[4];
    uint8_t bitsize;
    uint8_t endian;
    uint8_t version;
    uint8_t abi;
    uint64_t padding;
    uint16_t exe_type;
    uint16_t isa;
    uint32_t elf_version;
} ELF_common_header_t;

typedef struct ELF64_header {
    ELF_common_header_t common;
    uint64_t prog_entry_pos;
    uint64_t prog_table_pos;
    uint64_t sec_table_pos;
    uint32_t flags;
    uint16_t hdr_size;
    uint16_t prog_ent_size;
    uint16_t prog_ent_num;
    uint16_t sec_ent_size;
    uint16_t sec_ent_num;
    uint16_t sec_name_idx;
} ELF64_header_t;

typedef struct ELF64_prog_header {
    uint32_t type;
    uint32_t flags;
    uint64_t file_offset;
    uint64_t load_addr;
    uint64_t undefined;
    uint64_t file_size;
    uint64_t mem_size;
    uint64_t alignment;
} ELF64_prog_header_t;

virtual_addr_t ELF_mmap_binary(inode_t *root, char *path) {
    file_t *file;
    inode_t *inode;
    ELF64_header_t header;
    ELF64_prog_header_t prog_header;
    char *magic = "\177ELF";
    off_t cursor;
    int i;
    permission_t perms;

    inode = FS_inode_for_path(path, root);
    file = inode->open(inode);

    // Read ELF header
    file->read(file, (char *)&header, sizeof(ELF64_header_t));

    if (strncmp((char *)header.common.magic, magic, 4) != 0 ||
        header.common.bitsize != BITSIZE_64 ||
        header.common.endian != LITTLE_ENDIAN ||
        header.common.version != VERSION ||
        header.common.abi != X86_64 ||
        header.common.exe_type != EXECUTABLE) {
        printk("ELF_mmap_binary(): Unsupported binary\n");
        return 0;
    }

    // Read ELF program headers
    cursor = header.prog_table_pos;
    for (i = 0; i < header.prog_ent_num; i++) {
        // Read program header
        file->lseek(file, cursor + i * header.prog_ent_size);
        file->read(file, (char *)&prog_header, header.prog_ent_size);

        // Allocate associated addresses in virtual space
        perms.x = 1;
        perms.w = 1;
        user_allocate_range(prog_header.load_addr, prog_header.mem_size, perms);

        // Read section into memory
        file->lseek(file, prog_header.file_offset);
        file->read(file, (char *)prog_header.load_addr, prog_header.file_size);
    }

    file->close(&file);

    return header.prog_entry_pos;
}