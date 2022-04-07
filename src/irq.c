#include "irq.h"
#include "isr.h"
#include "printk.h"

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

void IRQ_init() {
    // Set ISRs in IDT
    IRQ_set_handler(DIVIDE_BY_ZERO, handler);
    IRQ_set_handler(DEBUG, handler);
    IRQ_set_handler(NON_MASKABLE_INTERRUPT, handler);
    IRQ_set_handler(BREAKPOINT, handler);
    IRQ_set_handler(OVERFLOW, handler);
    IRQ_set_handler(BOUND_RANGE_EXCEEDED, handler);
    IRQ_set_handler(INVALID_OPCODE, handler);
    IRQ_set_handler(DEVICE_NOT_AVAILABLE, handler);
    IRQ_set_handler(x87_FLOAT_POINT, handler);
    IRQ_set_handler(MACHINE_CHECK, handler);
    IRQ_set_handler(SIMD_FLOATING_POINT, handler);
    IRQ_set_handler(VIRTUALIZATION, handler);
    IRQ_set_err_handler(DOUBLE_FAULT, double_fault_handler);
    IRQ_set_err_handler(INVALID_TSS, invalid_tss_handler);
    IRQ_set_err_handler(SEGMENT_NOT_PRESENT, segment_not_present_handler);
    IRQ_set_err_handler(STACK_SEGMENT_FAULT, stack_segment_fault_handler);
    IRQ_set_err_handler(GENERAL_PROTECTION_FAULT, general_protection_fault_handler);
    IRQ_set_err_handler(PAGE_FAULT, page_fault_handler);
    IRQ_set_err_handler(ALIGNMENT_CHECK, alignment_check_handler);
    IRQ_set_err_handler(CONTROL_PROTECTION, control_protection_handler);
    IRQ_set_err_handler(SECURITY_EXCEPTION, security_exception_handler);

    // Setup IDT register
    idt_r.base = (uint64_t)&idt[0];
    idt_r.limit = (uint16_t)sizeof(idt_entry_t) * NUM_ENTRIES - 1;

    // Load the IDT
    asm volatile ("lidt %0" : : "m"(idt_r)); 

    // Set interrupt mask

    // Set interrupt flag
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