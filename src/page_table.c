#include "page_table.h"
#include "mmu.h"
#include "multiboot.h"
#include <stdint-gcc.h>
#include <stddef.h>
#include <stdbool.h>
#include "printk.h"
#include "registers.h"
#include "irq.h"
#include "string.h"

extern memory_map_t mmap;

extern void enable_no_execute(void);

extern uint8_t p4_table;
extern uint8_t p3_table;
extern uint8_t p2_table;
extern uint8_t stack_bottom;
extern uint8_t ist_stack1_bottom;
extern uint8_t ist_stack2_bottom;
extern uint8_t ist_stack3_bottom;

// Entry flag positions
#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER_ACCESS 0x4
#define PAGE_NO_EXECUTE 0x8000000000000000

// PML4 entries
#define PML4_IDENTITY_MAP 0
#define PAGE_OFFSET 12
#define VGA_ADDR 0xB8000

// Offsets in Virtual Address Space
#define SECTION_SIZE 0x7FFFFFFFFF

#define PAGE_FAULT_IRQ 14

#define ELF_WRITE_FLAG 0x1
#define ELF_EXEC_FLAG 0x4
#define GUARD_STACK_SIZE STACK_SIZE + PAGE_SIZE

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
    uint64_t avl1 : 3;
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
// Sets present bit, and provided flags
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
    *pt_entry = phys_addr | PAGE_PRESENT | flags;
}


// Identity maps a range of physical addresses in the PML4
// Sets present and writable flags
void identity_map(page_table_t *pml4, physical_addr_t start, physical_addr_t end) {
    printk("Identity mapped region: 0x%lx - 0x%lx\n", start, end - 1);
    for ( ; start < end; start += PAGE_SIZE) {
        map_page(pml4, start, start, PAGE_WRITABLE);
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
    char *action = (error_code & 0x2) ? "write" : "read";
    printk("PAGE FAULT: Invalid %s at %p\n", action, (void *)get_cr2());
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
        flags = 0;
        if (current->segment_size != 0) {
            // Set page flags based on type of elf section
            if (current->flags & ELF_WRITE_FLAG) flags |= PAGE_WRITABLE;
            if (!(current->flags & ELF_EXEC_FLAG)) flags |= PAGE_NO_EXECUTE;

            map_range(pml4, current->segment_address, KERNEL_TEXT_START + current->segment_address, current->segment_size, flags);
            printk("Mapped %s to 0x%lx\n", get_elf_section_name(current->section_name_index), KERNEL_TEXT_START + current->segment_address);
        }
    }
}

// Maps an existing stack in the physical address space to the virtual space
// Returns the address of the top of the stack
virtual_addr_t map_stack(page_table_t *pml4, virtual_addr_t vbottom, physical_addr_t pbottom) {
    virtual_addr_t vcurrent;

    // Leave room for a guard page
    for (vcurrent = vbottom + PAGE_SIZE; vcurrent < vbottom + GUARD_STACK_SIZE; vcurrent += PAGE_SIZE) {
        map_page(pml4, vcurrent, pbottom, PAGE_NO_EXECUTE | PAGE_WRITABLE);
        pbottom += PAGE_SIZE;
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
    map_page(pml4, VGA_ADDR, VGA_ADDR, PAGE_WRITABLE | PAGE_NO_EXECUTE);

    // Map ELF sections into kernel text region
    remap_elf_sections(pml4);

    // Map main stack and IST stacks
    stack_addresses[0] = map_stack(pml4, KERNEL_STACKS_START, (physical_addr_t)&stack_bottom);
    // stack_addresses[1] = map_stack(pml4, KERNEL_STACKS_START + stack_addresses[0], (physical_addr_t)&ist_stack1_bottom);
    // stack_addresses[2] = map_stack(pml4, KERNEL_STACKS_START + stack_addresses[1], (physical_addr_t)&ist_stack2_bottom);
    // stack_addresses[3] = map_stack(pml4, KERNEL_STACKS_START + stack_addresses[2], (physical_addr_t)&ist_stack3_bottom);

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