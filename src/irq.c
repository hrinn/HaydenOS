#include "irq.h"
#include "printk.h"

#define UNUSED __attribute__((unused))
#define FLAGS 0x8E // 1000 1110 
#define NUM_ENTRIES 256
#define NUM_HANDLERS 12
#define NUM_ERR_HANDLERS 9

typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t gdt_selector;  // Selects a code segment in the GDT
    uint8_t ist;            // IST in the TSS that the CPU will load into RSP
    uint8_t attributes;     // Type and attributes
    uint16_t isr_mid;       // Middle 16 bits of ISR's address
    uint32_t isr_high;      // Upper 32 bits of ISR's address
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit; // Size of IDT
    uint64_t base;  // Address of IDT
} __attribute__((packed)) idt_r_t;

// Interrupt descriptor table
__attribute__((aligned(16)))
static idt_entry_t idt[NUM_ENTRIES];

// IDT Descriptor Register
static idt_r_t idt_r;

__attribute__((interrupt))
void handler(UNUSED struct interrupt_frame *frame) {
    cli();
    printk("interrupt\n");
    sti();
}

__attribute__((interrupt))
void err_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("interrupt with error code: 0x%lx\n", error_code);
    sti();
}

void IRQ_init() {
    uint8_t isr_handlers[] = {DIVIDE_BY_ZERO, DEBUG, NON_MASKABLE_INTERRUPT, BREAKPOINT, \
        OVERFLOW, BOUND_RANGE_EXCEEDED, INVALID_OPCODE, DEVICE_NOT_AVAILABLE, \
        x87_FLOAT_POINT, MACHINE_CHECK, SIMD_FLOATING_POINT, VIRTUALIZATION};

    uint8_t isr_err_handlers[] = {DOUBLE_FAULT, INVALID_TSS, SEGMENT_NOT_PRESENT, \
        STACK_SEGMENT_FAULT, GENERAL_PROTECTION_FAULT, PAGE_FAULT, ALIGNMENT_CHECK, \
        CONTROL_PROTECTION, SECURITY_EXCEPTION};

    uint8_t i;

    // Setup IDT register
    idt_r.base = (uint64_t)&idt[0];
    idt_r.limit = (uint16_t)sizeof(idt_entry_t) * NUM_ENTRIES - 1;

    // Set ISRs in IDT
    for (i = 0; i < NUM_HANDLERS; i++) {
        IRQ_set_handler(isr_handlers[i], handler);
    }

    // Set ISRs with an error code
    for (i = 0; i < NUM_ERR_HANDLERS; i++) {
        IRQ_set_err_handler(isr_err_handlers[i], err_handler);
    }

    // Load the IDT and set interrupt flag
    asm volatile ("lidt %0" : : "m"(idt_r)); 
    sti();
}

static void set_isr(uint8_t irq, uint64_t handler_addr) {
    idt_entry_t *entry = &idt[irq];

    // Setup handler address
    entry->isr_low = handler_addr & 0xFFFF;
    entry->isr_mid = (handler_addr >> 16) & 0xFFFF;
    entry->isr_high = (handler_addr >> 32) & 0xFFFFFFFF;

    entry->gdt_selector = 0x8; // Offset of the kernel code selector in the GDT
    entry->ist = 0;
    entry->attributes = FLAGS;
    entry->reserved = 0;
}

void IRQ_set_handler(uint8_t irq, irq_handler_t handler) {
    set_isr(irq, (uint64_t)handler);
}

void IRQ_set_err_handler(uint8_t irq, irq_err_handler_t handler) {
    set_isr(irq, (uint64_t)handler);
}