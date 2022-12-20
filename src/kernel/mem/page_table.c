#include "page_table.h"
#include "memdef.h"
#include <stddef.h>
#include "multiboot.h"
#include "pf_alloc.h"
#include "string.h"
#include "printk.h"
#include "registers.h"
#include "irq.h"

#define NUM_ENTRIES 512

#define PAGE_PRESENT 0x1
#define PAGE_USER_ACCESS 0x4
#define PAGE_OFFSET 12

#define ELF_WRITE_FLAG 0x1
#define ELF_EXEC_FLAG 0x4

#define PAGE_FAULT_IRQ 14

#define VGA_ADDR 0xB8000

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

static page_table_t *pml4;
extern memory_map_t mmap;
extern void enable_no_execute(void);

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
void map_page(virtual_addr_t virt_addr, physical_addr_t phys_addr, uint64_t flags) {
    pt_index_t *i;
    i = (pt_index_t *)&virt_addr;
    page_table_t *pdp, *pd, *pt;
    uint64_t *pt_entry;

    if (!pml4->table[i->pml4_index].present) {
        pml4->table[i->pml4_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pml4->table[i->pml4_index].present = 1;
        if (flags & PAGE_WRITABLE) pml4->table[i->pml4_index].writable = 1;
        if (flags & PAGE_USER_ACCESS) pml4->table[i->pml4_index].user = 1;
    }
    pdp = entry_to_table(pml4, i->pml4_index);

    if (!pdp->table[i->pdp_index].present) {
        pdp->table[i->pdp_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pdp->table[i->pdp_index].present = 1;
        if (flags & PAGE_WRITABLE) pdp->table[i->pdp_index].writable = 1;
        if (flags & PAGE_USER_ACCESS) pdp->table[i->pdp_index].user = 1;
    }
    pd = entry_to_table(pdp, i->pdp_index);


    if (!pd->table[i->pd_index].present) {
        pd->table[i->pd_index].base_addr = ((physical_addr_t)allocate_table()) >> PAGE_OFFSET;
        pd->table[i->pd_index].present = 1;
        if (flags & PAGE_WRITABLE) pd->table[i->pd_index].writable = 1;
        if (flags & PAGE_USER_ACCESS) pd->table[i->pd_index].user = 1;
    }
    pt = entry_to_table(pd, i->pd_index);

    pt_entry = (uint64_t *)&pt->table[i->pt_index];
    *pt_entry = phys_addr | flags;
}


// Identity maps a range of physical addresses in the PML4
// Sets present and writable flags
void identity_map(physical_addr_t start, physical_addr_t end) {
    printk("Identity mapped region: [0x%lx, 0x%lx)\n", start, end);
    for ( ; start < end; start += PAGE_SIZE) {
        map_page(start, start, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE);
    }
}

// Maps a range of physical addresses in the PML4, starting at provided virtual address
// Sets present flag and provided flags
void map_range(physical_addr_t pstart, virtual_addr_t vstart, 
    uint64_t size, uint64_t flags) 
{
    virtual_addr_t vcurrent;
    physical_addr_t pcurrent = pstart;

    for (vcurrent = vstart; vcurrent < vstart + size; vcurrent += PAGE_SIZE) {
        map_page(vcurrent, pcurrent, flags);
        pcurrent += PAGE_SIZE;
    }
}

// Demand allocates a virtual address range for user access
// Returns address of top of stack
virtual_addr_t user_allocate_range(virtual_addr_t start, uint64_t size) {
    map_range(0, start, size, PAGE_ALLOCATED | PAGE_USER_ACCESS | PAGE_WRITABLE);
    return start + size;
}

// Returns the page frame associated with a virtual address if it is mapped in PML4
pt_entry_t *get_page_frame(virtual_addr_t addr) {
    pt_index_t *i = (pt_index_t *)&addr;
    page_table_t *pdp = entry_to_table(pml4, i->pml4_index);
    if (pdp == NULL) return NULL;
    page_table_t *pd = entry_to_table(pdp, i->pdp_index);
    if (pd == NULL) return NULL;
    page_table_t *pt = entry_to_table(pd, i->pd_index);
    if (pt == NULL) return NULL;
    return &pt->table[i->pt_index];
}

// Handles page faults
void page_fault_handler(uint8_t irq, uint32_t error_code, void *arg) {
    virtual_addr_t page = get_cr2();
    physical_addr_t pf;
    pt_entry_t *entry;

    entry = get_page_frame(page);

    if (entry != NULL && entry->allocated) {
        // On demand paging
        pf = MMU_pf_alloc();
        entry->base_addr = (pf >> PAGE_OFFSET);
        entry->present = 1;
        entry->allocated = 0;
        return;
    }

    printk("PAGE FAULT: Invalid memory access at %p\n", (void *)page);
    printk("- %s\n", (error_code & 0x1) ? "Page protection violation" : "Not-present page");
    printk("- %s\n", (error_code & 0x2) ? "Write" : "Read");
    printk("- %s\n", (error_code & 0x4) ? "User mode" : "Kernel mode");
    if (error_code & 0x08) printk("- Read reserved field in page table entry\n");
    if (error_code & 0x10) printk("- Instruction fetch\n");

    while (1) asm("hlt");
}

// Maps each Kernel ELF section into the Kernel Text region of the virtual memory space
// Sets writable and no execute as appropriate
void remap_elf_sections() {
    struct elf_section_header *current;
    uint64_t flags;

    printk("Mapping Kernel ELF sections\n");

    for (current = (struct elf_section_header *)(mmap.elf_tag + 1);
        (uint8_t *)current < (uint8_t *)mmap.elf_tag + mmap.elf_tag->size;
        current++)
    {
        flags = PAGE_PRESENT;
        if (current->segment_size != 0 && current->segment_address >= KERNEL_TEXT_START) {
            // Set page flags based on type of elf section
            if (current->flags & ELF_WRITE_FLAG) flags |= PAGE_WRITABLE;
            if (!(current->flags & ELF_EXEC_FLAG)) flags |= PAGE_NO_EXECUTE;

            map_range(current->segment_address - KERNEL_TEXT_START, current->segment_address, current->segment_size, flags);
            printk("Mapped %s (R%c%c) at 0x%lx\n", 
                get_elf_section_name(current->section_name_index), 
                (current->flags & ELF_WRITE_FLAG) ? 'W' : '_',
                (current->flags & ELF_EXEC_FLAG) ? 'X' : '_',
                current->segment_address
            );
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
        map_page(vcurrent, pcurrent, PAGE_PRESENT | PAGE_NO_EXECUTE | PAGE_WRITABLE);
        pcurrent += PAGE_SIZE;
    }

    printk("Mapped stack, top at 0x%lx\n", vcurrent);
    return vcurrent;
}

// Frees a page frame based on a virtual address
// Returns 1 on success, -1 on failure
int free_pf_from_virtual_addr(virtual_addr_t addr) {
    pt_entry_t *entry;
    physical_addr_t page_frame;

    if ((entry = get_page_frame(addr)) == NULL) {
        printk("free_pf_from_virtual_addr(): Tried to free not present frame\n");
        return -1;
    }

    page_frame = (physical_addr_t)(entry->base_addr << PAGE_OFFSET);

    if (entry->present) {
        // Page was demand allocated, must deallocate
        MMU_pf_free(page_frame);
        entry->present = 0;
    } else {
        // Page has yet to be allocated, reset flag
        entry->allocated = 0;
    }

    return 1;
}

// Sets up a new PML4 with identity map, kernel text, and kernel stack
// Loads PML4 into CR3
void setup_pml4(virtual_addr_t *stack_addresses) {
    struct free_mem_region *region;

    pml4 = allocate_table();
    printk("Created PML4 at physical address %p\n", (void *)pml4);

    // Register page fault handler
    IRQ_set_handler(PAGE_FAULT_IRQ, page_fault_handler, NULL);

    // Enable no execute flag in EFER
    enable_no_execute();

    // Identity map our physical memory
    region = mmap.first_region;
    while (region != NULL) {
        identity_map(region->start, region->end);
        region = region->next;
    }

    // Identity map VGA address
    map_page(VGA_ADDR, VGA_ADDR, PAGE_PRESENT | PAGE_WRITABLE | PAGE_NO_EXECUTE);

    // Map ELF sections into kernel text region
    remap_elf_sections(pml4);

    // Map main stack and IST stacks
    // stack_addresses[0] = map_stack(pml4, KERNEL_STACKS_START, (physical_addr_t)&stack_bottom);
    // stack_addresses[1] = map_stack(pml4, stack_addresses[0], (physical_addr_t)&ist_stack1_bottom);
    // stack_addresses[2] = map_stack(pml4, stack_addresses[1], (physical_addr_t)&ist_stack2_bottom);
    // stack_addresses[3] = map_stack(pml4, stack_addresses[2], (physical_addr_t)&ist_stack3_bottom);
    // thread_stack_brk = stack_addresses[3];

    printk("Loading new PML4...\n");
    set_cr3((physical_addr_t)pml4);
}

void free_multiboot_sections() {
    struct elf_section_header *current;
    physical_addr_t current_page;

    for (current = (struct elf_section_header *)(mmap.elf_tag + 1);
        (uint8_t *)current < (uint8_t *)mmap.elf_tag + mmap.elf_tag->size;
        current++)
    {
        if (current->segment_size != 0 && current->segment_address < KERNEL_TEXT_START) {
            printk("Freeing multiboot section: 0x%lx - 0x%lx\n", 
                current->segment_address, 
                current->segment_address + current->segment_size);
            for (current_page = current->segment_address; 
                current_page < current->segment_address + current->segment_size; 
                current += PAGE_SIZE) 
            {
                MMU_pf_free(current_page);
            }
        }
    }
}