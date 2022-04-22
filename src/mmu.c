#include "mmu.h"
#include "page_table.h"
#include "multiboot.h"
#include "printk.h"
#include <stddef.h>
#include <stdbool.h>
#include "string.h"
#include "registers.h"
#include "irq.h"

#define PAGE_FAULT_IRQ 14

// Structure to keep track of page frames
struct free_mem_region {
    physical_addr_t start;
    physical_addr_t end;
    struct free_mem_region *next;
};

struct free_pool_node {
    struct free_pool_node *next;
};

typedef struct mmap {
    physical_addr_t kernel_start;
    physical_addr_t kernel_end;
    physical_addr_t multiboot_start;
    physical_addr_t multiboot_end;
    struct free_mem_region *current_region;
    physical_addr_t current_page;
    struct free_pool_node *free_pool;
    int allocated_pages;
} memory_map_t;

// Physical memory map information
static memory_map_t mmap;

// Kernel's PML4 Table
// static page_table_t *pml4_table;

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

void push_free_pool_entry(struct free_pool_node *entry) {
    if (mmap.free_pool == NULL) {
        mmap.free_pool = entry;
    } else {
        entry->next = mmap.free_pool;
        mmap.free_pool = entry;
    }
}

struct free_pool_node *pop_free_pool_entry() {
    struct free_pool_node *res;
    if (mmap.free_pool == NULL) return NULL;
    res = mmap.free_pool;
    mmap.free_pool = mmap.free_pool->next;
    return res;
}

int get_allocated_pages() {
    return mmap.allocated_pages;
}

// Returns an address to a free page frame
physical_addr_t MMU_pf_alloc(void) {
    physical_addr_t page;
    struct free_pool_node *node;
    bool page_valid;

    // Check the free pool linked list first
    if ((node = pop_free_pool_entry()) != NULL) {
        mmap.allocated_pages++;
        return (physical_addr_t)node;
    }

    page_valid = false;
    while (!page_valid) {
        if (!range_contains_addr(mmap.current_page, mmap.current_region->start, mmap.current_region->end)) {
            // Page is outside of current memory region
            if (mmap.current_region->next == NULL) {
                printk("Tried to allocate address: %p\n", (void *)mmap.current_page);
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

    mmap.allocated_pages++;
    page = mmap.current_page;
    mmap.current_page += PAGE_SIZE;
    return page;
}

// Frees the page frame associated with the address pf
void MMU_pf_free(physical_addr_t pf) {
    pf &= ~(PAGE_SIZE - 1);
    struct free_pool_node *entry;

    // Write a free pool node at the beginning of the page
    entry = (struct free_pool_node *)pf;
    entry->next = NULL;

    // Add free pool entry to the linked list
    push_free_pool_entry(entry);
    mmap.allocated_pages--;
}

// Page Table Functions

// Allocates a new page frame for a page table
page_table_t *allocate_table() {
    page_table_t *table = (page_table_t *)MMU_pf_alloc();
    memset(table, 0, sizeof(page_table_t));
    return table;
}

page_table_t *entry_to_table(page_table_t *parent_table, int i) {
    return (page_table_t *)((physical_addr_t)parent_table->table[i].base_addr << PAGE_OFFSET);
}

typedef struct {
    uint64_t page_offset : 12;
    uint64_t pt_index : 9;
    uint64_t pd_index : 9;
    uint64_t pdp_index : 9;
    uint64_t pml4_index : 9;
    uint64_t sign_extension : 16;
} __attribute__((packed)) pt_index_t;

void map_page(page_table_t *pml4, virtual_addr_t virt_addr, physical_addr_t phys_addr) {
    pt_index_t *i;
    i = (pt_index_t *)&virt_addr;
    page_table_t *pdp, *pd, *pt;

    if (!pml4->table[i->pml4_index].present) {
        printk("Allocated PDP at PML4[%d]\n", i->pml4_index);
        pml4->table[i->pml4_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pml4->table[i->pml4_index].present = 1;
    }
    pdp = entry_to_table(pml4, i->pml4_index);

    if (!pdp->table[i->pdp_index].present) {
        pdp->table[i->pdp_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pdp->table[i->pdp_index].present = 1;
    }
    pd = entry_to_table(pdp, i->pdp_index);


    if (!pd->table[i->pd_index].present) {
        pd->table[i->pd_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pd->table[i->pd_index].present = 1;
    }
    pt = entry_to_table(pd, i->pd_index);

    pt->table[i->pt_index].base_addr = phys_addr >> PAGE_OFFSET;
    pt->table[i->pt_index].present = 1;
    pt->table[i->pt_index].writable = 1;
}

void identity_map(page_table_t *pml4, physical_addr_t start, physical_addr_t end) {
    for ( ; start < end; start += PAGE_SIZE) {
        map_page(pml4, start, start);
    }
}

void map_range(page_table_t *pml4, physical_addr_t start, physical_addr_t end, virtual_addr_t offset) {
    virtual_addr_t vcurrent, size = end - start;
    physical_addr_t pcurrent = start;

    for (vcurrent = offset; vcurrent < offset + size; vcurrent += PAGE_SIZE) {
        map_page(pml4, vcurrent, pcurrent);        
        pcurrent += PAGE_SIZE;
    }
}

pt_entry_t *get_page_frame(page_table_t *pml4, virtual_addr_t addr) {
    pt_index_t *i = (pt_index_t *)&addr;
    page_table_t *pdp = entry_to_table(pml4, i->pml4_index);
    if (pdp == NULL) return NULL;
    page_table_t *pd = entry_to_table(pdp, i->pdp_index);
    if (pd == NULL) return NULL;
    page_table_t *pt = entry_to_table(pd, i->pd_index);
    if (pt == NULL) return NULL;
    return &pt->table[i->pt_index];
}

void page_fault_handler(uint8_t irq, uint32_t error_code, void *arg) {
    char *action = (error_code & 0x2) ? "write" : "read";
    printk("PAGE FAULT: Invalid %s at %p\n", action, (void *)get_cr2());
    while (1) asm("hlt");
}

// void print_pml4(page_table_t *pml4, virtual_addr_t start, virtual_addr_t end) {
//     virtual_addr_t current;
//     bool mapped = false;
//     pt_index_t *i;
//     pt_entry_t *page;

//     for (current = start; current < end; current += PAGE_SIZE) {
//         i = (pt_index_t*)&current;
//         page = get_page_frame(pml4, current);
//         if (mapped) {
//             if (page == NULL || page->present == 0) {
//                 mapped = false;
//                 printk("0x%lx (PML4[%d])\n", current - 1, i->pml4_index);
//             }
//         } else {
//             if (page != NULL && page->present == 1) {
//                 mapped = true;
//                 printk("MAPPED 0x%lx - ", current);
//             }
//         }
//     }
// }

void setup_pml4() {
    struct free_mem_region *region = mmap.current_region;
    page_table_t *pml4 = allocate_table();
    printk("Created PML4 at physical address %p\n", (void *)pml4);

    // Register page fault handler
    IRQ_set_handler(PAGE_FAULT_IRQ, page_fault_handler, NULL);

    // Identity map our physical memory
    while (region != NULL) {
        identity_map(pml4, region->start, region->end);
        region = region->next;
    }

    // Identity map VGA address
    map_page(pml4, VGA_ADDR, VGA_ADDR);

    set_cr3((physical_addr_t)pml4);
}

