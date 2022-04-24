#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint-gcc.h>

// Structures to parse the multiboot tags
struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

// Entry in multiboot struct that contains ELF info
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

struct multiboot_elf_tag {
    uint32_t type;
    uint32_t size;
    uint32_t num_entries;
    uint32_t entry_size;
    uint32_t string_table_index;
};

void parse_multiboot_tags(struct multiboot_info *);
char *get_elf_section_name(int section_name_index);

#endif