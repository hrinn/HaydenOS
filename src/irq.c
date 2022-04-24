#include "irq.h"
#include "isr_wrapper.h"
#include "printk.h"
#include "registers.h"
#include "ioport.h"
#include "gdt.h"

// Interrupt configuration
#define INTERRUPT_GATE 0xE
#define NUM_IDT_ENTRIES 256

// PIC Ports
#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1

// PIC Commands
#define PIC_EOI 0x20

// PIC Remap
#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28
#define PIC_START 0x20
#define PIC_END 0x2F
#define ICW1_INIT 0x11
#define ICW4_8086 0x01

// IRQ Numbers
#define DOUBLE_FAULT 8
#define PAGE_FAULT 14
#define GENERAL_PROTECTION_FAULT 13

void IRQ_end_of_interrupt(uint8_t irq_line);

// IDT entry
typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t target_selector;  // Selects a code segment in the GDT

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
static idt_entry_t idt[NUM_IDT_ENTRIES];

// ISR table (indexed by assembly isr wrappers)
static struct {
    void *arg;
    irq_handler_t handler;
} irq_handler_table[NUM_IDT_ENTRIES];

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

void irq_handler(uint8_t irq, uint32_t error_code) {
    if (irq_handler_table[irq].handler) {
        irq_handler_table[irq].handler(irq, error_code, irq_handler_table[irq].arg);
    } else {
        // No entry set for this irq
        printk("Unhandled Interrupt %d (%s)\n", 
            irq, irq < 32 ? irq_name_table[irq] : "External/Trap");
        sti();
        while (1) asm("hlt");
    }
}

void set_idt_entry(uint8_t irq) {
    idt_entry_t *entry = &idt[irq];

    // Get associated ISR wrapper
    uint64_t wrapper_addr = (uint64_t)isr_wrapper_table[irq];

    // Setup IDT entry
    entry->isr_low = wrapper_addr & 0xFFFF;
    entry->isr_mid = (wrapper_addr >> 16) & 0xFFFF;
    entry->isr_high = (wrapper_addr >> 32) & 0xFFFFFFFF;

    entry->target_selector = KERNEL_CODE_OFFSET;
    entry->ist = 0;
    entry->res1 = 0;
    entry->type = INTERRUPT_GATE;
    entry->zero = 0;
    entry->dpl = 0;
    entry->present = 1;
    entry->res2 = 0;
}

void apply_idt_offset(uint64_t offset) {
    int i;
    uint64_t addr;
    idt_entry_t *entry;

    for (i = 0; i < NUM_IDT_ENTRIES; i++) {
        addr = (uint64_t)isr_wrapper_table[i];
        addr += offset;
        entry = &idt[i];

        entry->isr_low = addr & 0xFFFF;
        entry->isr_mid = (addr >> 16) & 0xFFFF;
        entry->isr_high = (addr >> 32) & 0xFFFFFFFF;
    }
}

void PIC_remap() {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT);
    io_wait();
	outb(PIC2_COMMAND, ICW1_INIT);
    io_wait();
	outb(PIC1_DATA, PIC1_OFFSET);
    io_wait();
	outb(PIC2_DATA, PIC2_OFFSET);
    io_wait();
	outb(PIC1_DATA, 0x4);
    io_wait();
	outb(PIC2_DATA, 0x2);
    io_wait();
 
	outb(PIC1_DATA, ICW4_8086);
    io_wait();
	outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void IRQ_init() {
    int i;

    // Set all entries in IDT
    for (i = 0; i < NUM_IDT_ENTRIES; i++) {
        set_idt_entry(i);
    }

    // Set separate ISTs for DF, PF, GF
    idt[DOUBLE_FAULT].ist = 1;
    idt[PAGE_FAULT].ist = 2;
    idt[GENERAL_PROTECTION_FAULT].ist = 3;

    // Load IDT register
    lidt(&idt[0], (sizeof(idt_entry_t) * NUM_IDT_ENTRIES) - 1);

    // Mask out all PIC interrupts
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    // Remap PIC
    PIC_remap();

    // Set interrupt flag
    sti();
}

void IRQ_set_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;

    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    value = inb(port) | (1 << irq_line);
    outb(port, value);
}

void IRQ_clear_mask(uint8_t irq_line) {
    uint16_t port;
    uint8_t value;
 
    if(irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }
    value = inb(port) & ~(1 << irq_line);
    outb(port, value);        
}

uint8_t IRQ_get_mask(uint8_t irq_line) {
    uint16_t port;
    
    if (irq_line < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq_line -= 8;
    }

    return inb(port) & (1 << irq_line);
}

void IRQ_end_of_interrupt(uint8_t irq_line) {
    if (irq_line > 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void IRQ_set_handler(uint8_t irq, irq_handler_t handler, void *arg) {
    irq_handler_table[irq].handler = handler;
    irq_handler_table[irq].arg = arg;
}