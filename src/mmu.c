#include "mmu.h"
#include "printk.h"

#define MULTIBOOT_TAG_TYPE_ELF 9
#define MULTIBOOT_TAG_TYPE_MMAP 6
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
} __attribute__((packed));

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

static inline uint32_t align_size(uint32_t size) {
    return (size + 7) & ~7;
}

void parse_elf_tag(struct multiboot_elf_tag *tag) {
    printk("Kernel ELF Section Table\n");

    printk("Size: %d, num entries: %hd, entry size: %hd\n",
        tag->size, tag->num_entries, tag->entry_size);
}

void parse_mmap_tag(struct multiboot_mmap_tag *tag) {
    struct multiboot_mmap_entry *entry;

    printk("Memory Map\n");

    for (entry = (struct multiboot_mmap_entry *)(tag + 1);
        (uint8_t *)entry < (uint8_t *)tag + tag->size;
        entry++)
        {
            if (entry->type == MMAP_ENTRY_FREE_TYPE) {
                printk("Available memory, start: 0x%lx, size: %ldKB\n", entry->addr, entry->len/1024);
            }
        }
}

void parse_multiboot_tags(struct multiboot_info *multiboot_tags) {
    struct multiboot_tag *tag;

    printk("Multiboot start: %p, size: %dB\n", (void *)multiboot_tags, multiboot_tags->total_size);

    for (tag = (struct multiboot_tag *)(multiboot_tags + 1);
        tag->type != MULTIBOOT_TAG_TYPE_END;
        tag = (struct multiboot_tag *)((uint8_t *)tag + align_size(tag->size)))
    {
        switch(tag->type) {
            case MULTIBOOT_TAG_TYPE_ELF: 
                parse_elf_tag((struct multiboot_elf_tag *)tag);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
                parse_mmap_tag((struct multiboot_mmap_tag *)tag);
                break;
            default: break;
        }
    }
}