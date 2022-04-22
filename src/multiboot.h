#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint-gcc.h>

#define MULTIBOOT_TAG_TYPE_ELF 9
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_END 0
#define MMAP_ENTRY_FREE_TYPE 1

// Structures to parse the multiboot tags
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

struct multiboot_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot_elf_tag {
    uint32_t type;
    uint32_t size;
    uint32_t num_entries;
    uint32_t entry_size;
    uint32_t string_table_index;
};

struct elf_section_header {
    uint32_t section_name_index;
    uint32_t type;
    uint64_t flags;
    uint64_t segment_address;
    uint64_t segment_disk_offset;
    uint64_t segment_size;
    uint32_t table_index_link;
    uint32_t extra_info;
    uint64_t address_alignment;
    uint64_t fixed_entry_size;
};

void parse_multiboot_tags(struct multiboot_info *);

#endif