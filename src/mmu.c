#include "mmu.h"
#include <stddef.h>
#include <stdbool.h>
#include "printk.h"
#include "irq.h"
#include "string.h"
#include "registers.h"
#include "error.h"

// Multiboot types
#define MULTIBOOT_TAG_TYPE_ELF 9
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_END 0
#define MMAP_ENTRY_FREE_TYPE 1

// Page table
#define NUM_ENTRIES 512
#define KERNEL_STACKS_START 0xffffff8000000000
#define KERNEL_HEAP_START 0xffff808000000000
#define KERNEL_PSTACKS_START 0xffffff0000000000

// Page table entry flags
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER_ACCESS 0x4
#define PAGE_ALLOCATED 0x200
#define PAGE_NO_EXECUTE 0x8000000000000000

// Addresses and offsets
#define SECTION_SIZE 0x7FFFFFFFFF
#define VGA_ADDR 0xB8000
#define PAGE_OFFSET 12
#define STACK_SIZE PAGE_SIZE * 2

// ELF section flags
#define ELF_WRITE_FLAG 0x1
#define ELF_EXEC_FLAG 0x4

// Interrupts
#define PAGE_FAULT_IRQ 14

// Structs for tracking physical and virtual system memory
struct free_pool_node {
    struct free_pool_node *next;
};

struct free_mem_region {
    physical_addr_t start;
    physical_addr_t end;
    struct free_mem_region *next;
};

struct addr_range {
    physical_addr_t start;
    physical_addr_t end;
};

typedef struct mmap {
    struct addr_range kernel;
    struct addr_range multiboot;
    struct free_mem_region *first_region;
    struct free_mem_region *current_region;
    physical_addr_t current_page;
    struct free_pool_node *free_pool;
    struct multiboot_elf_tag *elf_tag;
} memory_map_t;

// Multiboot tag structs
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

// Page table structures
typedef struct page_table_entry {
    uint64_t present : 1;
    uint64_t writable : 1;
    uint64_t user : 1;
    uint64_t write_through : 1;
    uint64_t disable_cache : 1;
    uint64_t accessed : 1;
    uint64_t dirty : 1;
    uint64_t huge : 1;
    uint64_t global : 1;
    uint64_t allocated : 1; // Replaced an available bit
    uint64_t avl1 : 2;
    uint64_t base_addr : 40;
    uint64_t avl2 : 11;
    uint64_t no_execute : 1;
} __attribute__((packed)) pt_entry_t;

typedef struct page_table {
    pt_entry_t table[NUM_ENTRIES];
} __attribute__((packed)) page_table_t;

typedef struct {
    uint64_t page_offset : 12;
    uint64_t pt_index : 9;
    uint64_t pd_index : 9;
    uint64_t pdp_index : 9;
    uint64_t pml4_index : 9;
    uint64_t sign_extension : 16;
} __attribute__((packed)) pt_index_t;

// Function definitions
void map_page(page_table_t *pml4, virtual_addr_t virt_addr, physical_addr_t phys_addr, uint64_t flags);
pt_entry_t *get_page_frame(page_table_t *pml4, virtual_addr_t addr);

// External labels and functions
extern void enable_no_execute(void);
extern uint8_t p4_table;
extern uint8_t p3_table;
extern uint8_t p2_table;
extern uint8_t stack_bottom;
extern uint8_t ist_stack1_bottom;
extern uint8_t ist_stack2_bottom;
extern uint8_t ist_stack3_bottom;

// Static variables
static memory_map_t mmap;
static page_table_t *pml4;
static virtual_addr_t kernel_brk = KERNEL_HEAP_START; // Next available page

static inline uint64_t align_page(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

// Memory management interface functions /////////////////////////////

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

// Allocates a physical page frame
physical_addr_t MMU_pf_alloc(void) {
    physical_addr_t page;
    struct free_pool_node *node;
    bool page_valid;

    // Check the free pool linked list first
    if ((node = pop_free_pool_entry()) != NULL) {
        return (physical_addr_t)node;
    }

    page_valid = false;
    while (!page_valid) {
        if (!range_contains_addr(mmap.current_page, mmap.current_region->start, mmap.current_region->end)) {
            // Page is outside of current memory region
            if (mmap.current_region->next == NULL) {
                blue_screen("MMU_pf_alloc: No physical memory remaining");
                while (1) asm("hlt");
            }
            // Set current free entry to next
            mmap.current_region = mmap.current_region->next;
            mmap.current_page = align_page(mmap.current_region->start);
        } 
        
        if (range_contains_addr(mmap.current_page, mmap.kernel.start, mmap.kernel.end)) {
            // Page is inside of kernel
            mmap.current_page = align_page(mmap.kernel.end);
            continue; // Recheck address
        }
        
        if (range_contains_addr(mmap.current_page, mmap.multiboot.start, mmap.multiboot.end)) {
            mmap.current_page = align_page(mmap.multiboot.end);
            continue; // Recheck address
        }

        page_valid = true;
    }

    page = mmap.current_page;
    mmap.current_page += PAGE_SIZE;
    return page;
}

// Returns a physical page frame to the free pool
void MMU_pf_free(physical_addr_t pf) {
    pf &= ~(PAGE_SIZE - 1);
    struct free_pool_node *entry;

    // Write a free pool node at the beginning of the page
    entry = (struct free_pool_node *)pf;
    entry->next = NULL;

    // Add free pool entry to the linked list
    push_free_pool_entry(entry);
}

void *MMU_alloc_page() {
    virtual_addr_t address;

    if (kernel_brk >= KERNEL_PSTACKS_START) {
        printk("MMU_alloc_page: exhausted terabytes of kernel heap space!\n");
        return NULL;
    }

    // Map a page at the current heap top
    // Leave this page as not present, but set allocated bit for on demand paging
    map_page(pml4, kernel_brk, 0, PAGE_ALLOCATED | PAGE_WRITABLE | PAGE_NO_EXECUTE);
    
    address = kernel_brk;
    kernel_brk += PAGE_SIZE;
    return (void *)address;
}

void *MMU_alloc_pages(int num) {
    int i;
    void *start_address, *temp;

    for (i = 0; i < num; i++) {
        temp = MMU_alloc_page();
        if (i == 0) start_address = temp;
    }

    return start_address;
}

void MMU_free_page(void *address) {
    virtual_addr_t vaddr = (virtual_addr_t)address;
    physical_addr_t pf;
    pt_entry_t *entry;

    if (vaddr < KERNEL_HEAP_START || vaddr >= KERNEL_PSTACKS_START) {
        printk("MMU_free_page: tried to free an invalid address\n");
        return;
    }

    entry = get_page_frame(pml4, vaddr);
    pf = (physical_addr_t)(entry->base_addr << PAGE_OFFSET);

    if (entry->present == 1) {
        // Page was allocated on demand, must deallocate
        MMU_pf_free(pf);
    }

    entry->present = 0;
    entry->allocated = 0;

    // There is so much space available in the virtual memory
    // That we don't have to track freed pages
}

void MMU_free_pages(void *start_address, int num) {
    int i;

    for (i = 0; i < num; i++) {
        MMU_free_page(start_address);
        start_address = (void *)(((virtual_addr_t)start_address) + PAGE_SIZE);
    }
}

// Functions for parsing multiboot tags /////////////////////////////

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

    for (header = (struct elf_section_header *)(tag + 1);
        (uint8_t *)header < (uint8_t *)tag + tag->size;
        header++)
    {
        // Compute min start and max end address of kernel's ELF sections
        if (header->segment_size != 0) {
            if (header->segment_address < mmap.kernel.start) {
                mmap.kernel.start = header->segment_address;
            }
            if (header->segment_address + header->segment_size > mmap.kernel.end) {
                mmap.kernel.end = header->segment_address + header->segment_size;
            }
        }
    }

    mmap.elf_tag = tag;
}

void append_free_mem_region(struct free_mem_region *entry) {
    struct free_mem_region *current;

    if (mmap.first_region == NULL) {
        mmap.first_region = entry;
        mmap.current_region = mmap.first_region;
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
        printk("Free memory start: 0x%lx, end 0x%lx, len: %ldKB\n", 
            current->start, current->end, (current->end - current->start)/1024);
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
            if (mod_entry->start == 0x0) {
                if (mod_entry->end <= PAGE_SIZE) continue;  // This is not a valid free region
                mod_entry->start = PAGE_SIZE;   // Ensure free region doesn't start at 0x0
            }
            mod_entry->next = NULL;  // This overwrites the multiboot type and zero fields
            append_free_mem_region(mod_entry);
        }
    }
}

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
    printk("Kernel start: 0x%lx, end: 0x%lx, len: %ldKB\n", 
        mmap.kernel.start, mmap.kernel.end, (mmap.kernel.end - mmap.kernel.start)/1024);
    printk("Multiboot start: 0x%lx, end: 0x%lx, len: %ldB\n", 
        mmap.multiboot.start, mmap.multiboot.end, mmap.multiboot.end - mmap.multiboot.start);
    print_free_mem_regions();

    mmap.current_page = align_page(mmap.first_region->start);
    mmap.free_pool = NULL;
}

// Functions for managing page tables /////////////////////////////

// Allocates a new page frame for a page table
page_table_t *allocate_table() {
    page_table_t *table = (page_table_t *)MMU_pf_alloc();
    memset(table, 0, sizeof(page_table_t));
    return table;
}

// Gets page table pointer from page table entry
page_table_t *entry_to_table(page_table_t *parent_table, int i) {
    return (page_table_t *)((physical_addr_t)parent_table->table[i].base_addr << PAGE_OFFSET);
}

// Takes a virtual address and maps it to a physical address in the provided PML4
// Sets provided flags
void map_page(page_table_t *pml4, virtual_addr_t virt_addr, physical_addr_t phys_addr, uint64_t flags) {
    pt_index_t *i;
    i = (pt_index_t *)&virt_addr;
    page_table_t *pdp, *pd, *pt;
    uint64_t *pt_entry;

    if (!pml4->table[i->pml4_index].present) {
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

    pt_entry = (uint64_t *)&pt->table[i->pt_index];
    *pt_entry = phys_addr | flags;
}


// Identity maps a range of physical addresses in the PML4
// Sets present and writable flags
void identity_map(page_table_t *pml4, physical_addr_t start, physical_addr_t end) {
    printk("Identity mapped region: 0x%lx - 0x%lx\n", start, end - 1);
    for ( ; start < end; start += PAGE_SIZE) {
        map_page(pml4, start, start, PAGE_PRESENT | PAGE_WRITABLE);
    }
}

// Maps a range of physical addresses in the PML4, starting at provided virtual address
// Sets present flag and provided flags
void map_range(page_table_t *pml4, physical_addr_t pstart, virtual_addr_t vstart, 
    uint64_t size, uint64_t flags) 
{
    virtual_addr_t vcurrent;
    physical_addr_t pcurrent = pstart;

    for (vcurrent = vstart; vcurrent < vstart + size; vcurrent += PAGE_SIZE) {
        map_page(pml4, vcurrent, pcurrent, flags);
        pcurrent += PAGE_SIZE;
    }
}

// Returns the page frame associated with a virtual address if it is mapped in PML4
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

void set_page_noexec(page_table_t *pml4, virtual_addr_t addr) {
    pt_entry_t *pf = get_page_frame(pml4, addr);
    if (pf == NULL) return;
    pf->no_execute = 1;
}

// Handles page faults
void page_fault_handler(uint8_t irq, uint32_t error_code, void *arg) {
    virtual_addr_t page = get_cr2();
    physical_addr_t pf;
    char *action;
    pt_entry_t *entry;

    entry = get_page_frame(pml4, page);

    if (entry != NULL && entry->allocated) {
        // On demand paging
        pf = MMU_pf_alloc();
        entry->base_addr = (pf >> PAGE_OFFSET);
        entry->present = 1;
        return;
    }

    action = (error_code & 0x2) ? "write" : "read";
    printk("PAGE FAULT: Invalid %s at %p\n", action, (void *)page);
    while (1) asm("hlt");
}

// Maps each Kernel ELF section into the Kernel Text region of the virtual memory space
// Sets writable and no execute as appropriate
void remap_elf_sections(page_table_t *pml4) {
    struct elf_section_header *current;
    uint64_t flags;

    printk("Remapping Kernel ELF sections\n");

    for (current = (struct elf_section_header *)(mmap.elf_tag + 1);
        (uint8_t *)current < (uint8_t *)mmap.elf_tag + mmap.elf_tag->size;
        current++)
    {
        flags = PAGE_PRESENT;
        if (current->segment_size != 0) {
            // Set page flags based on type of elf section
            if (current->flags & ELF_WRITE_FLAG) flags |= PAGE_WRITABLE;
            if (!(current->flags & ELF_EXEC_FLAG)) flags |= PAGE_NO_EXECUTE;

            map_range(pml4, current->segment_address, KERNEL_TEXT_START + current->segment_address, current->segment_size, flags);
            printk("Mapped %s to 0x%lx (%lx)\n", get_elf_section_name(current->section_name_index), KERNEL_TEXT_START + current->segment_address, current->flags);
        }
    }
}

// Maps an existing stack in the physical address space to the virtual space
// Returns the address of the top of the stack
virtual_addr_t map_stack(page_table_t *pml4, virtual_addr_t vbottom, physical_addr_t pbottom) {
    virtual_addr_t vcurrent;
    physical_addr_t pcurrent = pbottom;

    // Leave room for a guard page
    for (vcurrent = vbottom + PAGE_SIZE; vcurrent < vbottom + STACK_SIZE + PAGE_SIZE; vcurrent += PAGE_SIZE) {
        map_page(pml4, vcurrent, pcurrent, PAGE_PRESENT | PAGE_NO_EXECUTE | PAGE_WRITABLE);
        pcurrent += PAGE_SIZE;
    }

    printk("Mapped stack, top at 0x%lx\n", vcurrent);
    return vcurrent;
}

// Sets up a new PML4 with identity map, kernel text, and kernel stack
// Loads PML4 into CR3
void setup_pml4(virtual_addr_t *stack_addresses) {
    struct free_mem_region *region = mmap.first_region;
    pml4 = allocate_table();
    printk("Created PML4 at physical address %p\n", (void *)pml4);

    // Register page fault handler
    IRQ_set_handler(PAGE_FAULT_IRQ, page_fault_handler, NULL);

    // Enable no execute flag in EFER
    enable_no_execute();

    // Identity map our physical memory
    while (region != NULL) {
        identity_map(pml4, region->start, region->end);
        region = region->next;
    }

    // Identity map VGA address
    map_page(pml4, VGA_ADDR, VGA_ADDR, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE);

    // Map ELF sections into kernel text region
    remap_elf_sections(pml4);

    // Map main stack and IST stacks
    stack_addresses[0] = map_stack(pml4, KERNEL_STACKS_START, (physical_addr_t)&stack_bottom);
    stack_addresses[1] = map_stack(pml4, stack_addresses[0], (physical_addr_t)&ist_stack1_bottom);
    stack_addresses[2] = map_stack(pml4, stack_addresses[1], (physical_addr_t)&ist_stack2_bottom);
    stack_addresses[3] = map_stack(pml4, stack_addresses[2], (physical_addr_t)&ist_stack3_bottom);

    printk("Loading new PML4...\n");
    set_cr3((physical_addr_t)pml4);
}

void cleanup_old_virtual_space() {
    struct free_mem_region *region = mmap.first_region;
    virtual_addr_t current;

    // Free page tables
    MMU_pf_free((physical_addr_t)&p4_table);
    MMU_pf_free((physical_addr_t)&p3_table);
    MMU_pf_free((physical_addr_t)&p2_table);

    // Remap identity region to no execute
    while (region != NULL) {
        for (current = region->start; current < region->end; current += PAGE_SIZE) {
            set_page_noexec(pml4, current);
        }
        region = region->next;
    }
}