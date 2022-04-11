#include "irq.h"
#include "isr_wrapper.h"
#include "printk.h"
#include "table_register.h"

#define INTERRUPT_GATE 0xE
#define NUM_ENTRIES 256

// IDT entry
typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t gdt_selector;  // Selects a code segment in the GDT

    uint8_t ist : 3;        // Index of Interrupt Stack Table
    uint8_t res1 : 5;
    
    uint8_t type : 4;       // 0xE (interrupt gate), 0xF (trap gate)
    uint8_t zero : 1;
    uint8_t dpl : 2;        // Protection / privilege level
    uint8_t present : 1;    // Indicates valid table entry

    uint16_t isr_mid;       // Middle 16 bits of ISR's address
    uint32_t isr_high;      // Upper 32 bits of ISR's address
    uint32_t res2;
} __attribute__((packed)) idt_entry_t;

// Interrupt descriptor table
__attribute__((aligned(16)))
static idt_entry_t idt[NUM_ENTRIES];

// ISR table (indexed by assembly isr wrappers)
irq_handler_t irq_handler_table[NUM_ENTRIES];

// IRQ name table
char *irq_name_table[32] = {
    "Divide-By-Zero-Error", "Debug", "Non-Maskable-Interrupt", "Breakpoint",
    "Overflow", "Bound-Range", "Invalid-Opcode", "Device-Not-Available",
    "Double-Fault", "Coprocessor-Segment-Overrun (Unsupported)", "Invalid-TSS",
    "Segment-Not-Present", "Stack", "General-Protection", "Page-Fault", "Reserved",
    "x87 Floating-Point Exception-Pending", "Alignment-Check", "Machine-Check",
    "SIMD Floating-Point", "Reserved", "Control-Protection-Exception", "Reserved",
    "Reserved", "Reserved", "Reserved", "Reserved", "Reserved",
    "Hypervisor Injection Exception", "VMM Communication Exception", 
    "Security Exception", "Reserved"
};

void default_handler(int irq, int error_code, void *arg) {
    printk("Unhandled Interrupt %d", irq);
    if (irq < 32) {
        printk(" (%s)\n", irq_name_table[irq]);
    } else {
        printk(" (External Interrupt)\n");
    }
}

void IRQ_init() {
    int i;

    // Set default handlers for the 32 system IRQs
    for (i = 0; i < 32; i++) {
        IRQ_set_handler(i, default_handler);
    }

    // Load IDT register
    lidt(&idt[0], sizeof(idt_entry_t) * NUM_ENTRIES - 1);

    // Set interrupt mask (PIC)

    // Set interrupt flag
    sti();
}

void IRQ_set_handler(uint8_t irq, irq_handler_t handler) {
    idt_entry_t *entry = &idt[irq];

    // Get associated ISR wrapper
    uint64_t wrapper_addr = (uint64_t)isr_wrapper_table[irq];

    // Setup IDT entry
    entry->isr_low = wrapper_addr & 0xFFFF;
    entry->isr_mid = (wrapper_addr >> 16) & 0xFFFF;
    entry->isr_high = (wrapper_addr >> 32) & 0xFFFFFFFF;

    entry->gdt_selector = 8; // Offset of the kernel code selector in the GDT
    entry->ist = 0;
    entry->type = INTERRUPT_GATE;
    entry->zero = 0;
    entry->dpl = 0;
    entry->present = 1;
    entry->res2 = 0;

    // Put handler in handler table
    irq_handler_table[irq] = handler;
}