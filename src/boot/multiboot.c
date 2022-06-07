#include "multiboot.h"
#include "loader.h"
#include "stub_elf.h"

#define MULTIBOOT_TAG_TYPE_ELF 9
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_END 0
#define MMAP_ENTRY_FREE_TYPE 1

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

struct multiboot_elf_tag {
    uint32_t type;
    uint32_t size;
    uint32_t num_entries;
    uint32_t entry_size;
    uint32_t string_table_index;
};

struct multiboot_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot_module_tag {
    uint32_t type;
    uint32_t size;
    uint32_t mod_start;
    uint32_t mod_end;
    char *cmdline;
};

typedef struct loader_info loader_info_t;

static inline uint32_t align_size(uint32_t size) {
    return (size + 7) & ~7;
}

void parse_module_tag(struct multiboot_module_tag *tag) {
    // If this is a binary, load it into memory at the correct location
    ELF_mmap_module(tag->mod_start, tag->mod_end);
}

void parse_multiboot_tags(struct multiboot_info *multiboot_tags, loader_info_t *loader_info) {
    struct multiboot_tag *tag;

    loader_info->multiboot.start = (uint64_t)multiboot_tags;
    loader_info->multiboot.end =  loader_info->multiboot.start + multiboot_tags->total_size;

    for (tag = (struct multiboot_tag *)(multiboot_tags + 1);
        tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag *)((uint8_t *)tag + align_size(tag->size)))
    {
        switch(tag->type) {
            case MULTIBOOT_TAG_TYPE_MODULE:
                parse_module_tag((struct multiboot_module_tag *)tag);
                break;
            case MULTIBOOT_TAG_TYPE_ELF: 
                // parse_elf_tag((struct multiboot_elf_tag *)tag);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                // parse_mmap_tag((struct multiboot_mmap_tag *)tag);
                break;
            default: break;
        }
    }
}