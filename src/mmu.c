#include "mmu.h"
#include "printk.h"
#include <stddef.h>
#include <stdbool.h>

#define MULTIBOOT_TAG_TYPE_ELF 9
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_END 0
#define MMAP_ENTRY_FREE_TYPE 1

// Structure to keep track of page frames
struct free_mem_region {
    uint64_t start;
    uint64_t end;
    struct free_mem_region *next;
};

struct free_pool_node {
    struct free_pool_node *next;
};

typedef struct mmap {
    uint64_t kernel_start;
    uint64_t kernel_end;
    uint64_t multiboot_start;
    uint64_t multiboot_end;
    struct free_mem_region *current_region;
    uint64_t current_page;
    struct free_pool_node *free_pool;
} memory_map_t;

// Structures to parse the multiboot tags
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

// Use for keeping track of memory
static memory_map_t mmap;

static inline uint32_t align_size(uint32_t size) {
    return (size + 7) & ~7;
}

void parse_elf_tag(struct multiboot_elf_tag *tag) {
    struct elf_section_header *header;

    for (header = (struct elf_section_header *)(tag + 1);
        (uint8_t *)header < (uint8_t *)tag + tag->size;
        header++)
    {
        // Compute min start and max end address of kernel's ELF sections
        if (header->segment_size != 0) {
            if (header->segment_address < mmap.kernel_start) {
                mmap.kernel_start = header->segment_address;
            }
            if (header->segment_address + header->segment_size > mmap.kernel_end) {
                mmap.kernel_end = header->segment_address + header->segment_size;
            }
        }
    }
}

void append_free_mem_region(struct free_mem_region *entry) {
    struct free_mem_region *current;

    if (mmap.current_region == NULL) {
        mmap.current_region = entry;
    } else {
        // Find end of linked list
        current = mmap.current_region;
        while (current->next) current = current->next;
        current->next = entry;
    }
}

void print_free_mem_regions() {
    struct free_mem_region *current = mmap.current_region;

    while (current) {
        printk("Free memory start: 0x%lx, end 0x%lx, len: %ldKB\n", 
            current->start, current->end, (current->end - current->start)/1024);
        current = current->next;
    }
}

void parse_mmap_tag(struct multiboot_mmap_tag *tag) {
    struct multiboot_mmap_entry *entry;
    struct free_mem_region *mod_entry;
    mmap.kernel_start = 0xFFFFFFFFFFFFFFFF;
    mmap.kernel_end = 0;

    for (entry = (struct multiboot_mmap_entry *)(tag + 1);
        (uint8_t *)entry < (uint8_t *)tag + tag->size;
        entry++)
    {
        if (entry->type == MMAP_ENTRY_FREE_TYPE) {            
            // Manipulate entry to our advantage.
            // Use memory to create a linked list of free memory regions
            mod_entry = (struct free_mem_region *)entry;
            mod_entry->end = entry->addr + entry->len;  // Change length to end address
            if (mod_entry->start == 0x0) {
                if (mod_entry->end <= PAGE_SIZE) continue;  // This is not a valid free region
                mod_entry->start = PAGE_SIZE;   // Ensure free region doesn't start at 0x0
            }
            mod_entry->next = NULL;  // This overwrites the multiboot type and zero fields
            append_free_mem_region(mod_entry);
        }
    }
}

static inline uint64_t align_page(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

void parse_multiboot_tags(struct multiboot_info *multiboot_tags) {
    struct multiboot_tag *tag;

    mmap.multiboot_start = (uint64_t)multiboot_tags;
    mmap.multiboot_end = mmap.multiboot_start + multiboot_tags->total_size;

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
    printk("Kernel start: 0x%lx, end: 0x%lx, len: %ldKB\n", 
        mmap.kernel_start, mmap.kernel_end, (mmap.kernel_end - mmap.kernel_start)/1024);
    printk("Multiboot start: 0x%lx, end: 0x%lx, len: %ldB\n", 
        mmap.multiboot_start, mmap.multiboot_end, mmap.multiboot_end - mmap.multiboot_start);
    print_free_mem_regions();

    mmap.current_page = align_page(mmap.current_region->start);
    mmap.free_pool = NULL;
}

// Returns true if the address is within the range
static inline bool range_contains_addr(uint64_t addr, uint64_t start, uint64_t end) {
    return ((addr - start) < (end - start));
}

// Returns an address to a free page frame
// This may not be robust enough
void *MMU_pf_alloc(void) {
    uint64_t page;
    bool page_valid;

    // Check the free pool linked list first
    if (mmap.free_pool != NULL) {
        // Get head of free pool's address
        page = (uint64_t)mmap.free_pool;
        mmap.free_pool = mmap.free_pool->next;
        return (void *)page;
    }

    page_valid = false;
    while (!page_valid) {
        if (!range_contains_addr(mmap.current_page, mmap.current_region->start, mmap.current_region->end)) {
            // Page is outside of current memory region
            if (mmap.current_region->next == NULL) {
                printk("ERROR: NO PHYSICAL MEMORY REMAINING!\n");
                while (1) asm("hlt");
            }
            // Set current free entry to next
            mmap.current_region = mmap.current_region->next;
            mmap.current_page = align_page(mmap.current_region->start);
        } 
        
        if (range_contains_addr(mmap.current_page, mmap.kernel_start, mmap.kernel_end)) {
            // Page is inside of kernel
            mmap.current_page = align_page(mmap.kernel_end);
            continue; // Recheck address
        }
        
        if (range_contains_addr(mmap.current_page, mmap.multiboot_start, mmap.multiboot_end)) {
            mmap.current_page = align_page(mmap.multiboot_end);
            continue; // Recheck address
        }

        page_valid = true;
    }

    page = mmap.current_page;
    mmap.current_page += PAGE_SIZE;
    return (void *)page;
}

void append_free_pool_entry(struct free_pool_node *entry) {
    struct free_pool_node *current;

    if (mmap.free_pool == NULL) {
        mmap.free_pool = entry;
    } else {
        // Find end of entry list
        current = mmap.free_pool;
        while (current->next) current = current->next;
        current->next = entry;
    }
}

// Frees the page frame associated with the address pf
void MMU_pf_free(void *pf) {
    uint64_t page_frame = (uint64_t)pf & ~(PAGE_SIZE - 1);
    struct free_pool_node *entry;

    // Write a free pool node at the beginning of the page
    entry = (struct free_pool_node *)page_frame;
    entry->next = NULL;

    // Add free pool entry to the linked list
    append_free_pool_entry(entry);
}