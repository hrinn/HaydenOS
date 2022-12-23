#include "multiboot.h"
#include <stddef.h>
#include "memdef.h"
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
};

struct multiboot_mmap_tag {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

// Global
memory_map_t mmap;

static inline uint32_t align_size(uint32_t size) {
    return (size + 7) & ~7;
}

char *get_elf_section_name(int section_name_index) {
    struct elf_section_header *strtab_header;
    char *strtab;

    strtab_header = ((struct elf_section_header *)(mmap.elf_tag + 1)) + mmap.elf_tag->string_table_index;
    strtab = (char *)strtab_header->segment_address;

    return strtab + section_name_index;
}

void parse_elf_tag(struct multiboot_elf_tag *tag) {
    struct elf_section_header *header;
    uint64_t physical_segment_address;

    for (header = (struct elf_section_header *)(tag + 1);
        (uint8_t *)header < (uint8_t *)tag + tag->size;
        header++)
    {
        // Compute min start and max end address of kernel's ELF sections
        if (header->segment_size != 0) {
            // Adjust virtual addresses into physical memory
            physical_segment_address = header->segment_address;
            if (header->segment_address > KERNEL_TEXT_START) {
                physical_segment_address -= KERNEL_TEXT_START;
            }

            if (physical_segment_address < mmap.kernel.start) {
                mmap.kernel.start = physical_segment_address;
            }
            if (physical_segment_address + header->segment_size > mmap.kernel.end) {
                mmap.kernel.end = physical_segment_address + header->segment_size;
            }
        }
    }

    mmap.elf_tag = tag;
}

void append_free_mem_region(struct free_mem_region *entry) {
    struct free_mem_region *current;

    if (mmap.first_region == NULL) {
        mmap.first_region = entry;
    } else {
        // Find end of linked list
        current = mmap.first_region;
        while (current->next) current = current->next;
        current->next = entry;
    }
}

void print_free_mem_regions() {
    struct free_mem_region *current = mmap.first_region;

    while (current) {
        printk("Free memory start: 0x%lx, end: 0x%lx, len: %ld pages\n", 
            current->start, current->end, (current->end - current->start) / PAGE_SIZE);
        current = current->next;
    }
}

void parse_mmap_tag(struct multiboot_mmap_tag *tag) {
    struct multiboot_mmap_entry *entry;
    struct free_mem_region *mod_entry;
    mmap.kernel.start = 0xFFFFFFFFFFFFFFFF;
    mmap.kernel.end = 0;

    for (entry = (struct multiboot_mmap_entry *)(tag + 1);
        (uint8_t *)entry < (uint8_t *)tag + tag->size;
        entry++)
    {
        if (entry->type == MMAP_ENTRY_FREE_TYPE) {            
            // Manipulate entry to our advantage.
            // Use memory to create a linked list of free memory regions
            mod_entry = (struct free_mem_region *)entry;
            mod_entry->end = entry->addr + entry->len;  // Change length to end address
            mod_entry->next = NULL;  // This overwrites the multiboot type and zero fields
            append_free_mem_region(mod_entry);
        }
    }
}

// Parses the multiboot tags and populates the mmap struct
// with memory information
void parse_multiboot_tags(struct multiboot_info *multiboot_tags) {
    struct multiboot_tag *tag;

    mmap.multiboot.start = (uint64_t)multiboot_tags;
    mmap.multiboot.end = mmap.multiboot.start + multiboot_tags->total_size;

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

    // Print gathered information
    printk("Physical memory information:\n");
    printk("Kernel start: 0x%lx, end: 0x%lx, len: %ld KB\n", 
        mmap.kernel.start, mmap.kernel.end, (mmap.kernel.end - mmap.kernel.start)/1024);
    printk("Multiboot start: 0x%lx, end: 0x%lx, len: %ld B\n", 
        mmap.multiboot.start, mmap.multiboot.end, mmap.multiboot.end - mmap.multiboot.start);
    print_free_mem_regions();
}