#include "irq.h"
#include "printk.h"

#define INT __attribute__((interrupt))
#define UNUSED __attribute__((unused))

typedef struct {
    unsigned int ist : 3;   // Indexes the interrupt stack table
    unsigned int reserved : 5;
    unsigned int gate : 1;  // 0: interrupt gate, 1: trap gate
    unsigned int one : 3;   
    unsigned int zero : 1;
    unsigned int dpl : 2;   // Descriptor privilege level: minimum privilege level required for calling this isr
    unsigned int present : 1;
} __attribute__((packed)) isr_options_t;

typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t gdt_selector;  // Selects a code segment in the GDT
    isr_options_t options;
    uint16_t isr_mid;       // Middle 16 bits of ISR's address
    uint32_t isr_high;      // Upper 32 bits of ISR's address
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

// Interrupt descriptor table
__attribute__((aligned(16)))
static idt_entry_t idt[256];

// IDT Register
static idtr_t idtr;

INT void breakpoint_handler(UNUSED struct interrupt_frame *frame, UNUSED unsigned long int error_code) {
    printk("breakpoint!\n");
}

void IRQ_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt);

    // Set ISRs in IDT
    // IRQ_set_handler(BREAKPOINT, breakpoint_handler, 0);
    // Configure PIC

    asm volatile ("lidt %0" : : "m"(idtr)); // Load the IDT
    sti(); // Set the interrupt flag
}

// Mask determines which interrupts are enabled
// void IRQ_set_mask(int irq) {

// }


// void IRQ_clear_mask(int irq) {

// }

// int IRQ_get_mask(int IRQline) {

// }

// void IRQ_end_of_interrupt(int irq) {

// }

void IRQ_set_handler(uint8_t irq, irq_handler_t handler, UNUSED void *arg) {

    // Setup handler address
    idt[irq].isr_low = (uint64_t)handler & 0xFFFF;
    idt[irq].isr_mid = ((uint64_t)handler >> 16) & 0xFFFF;
    idt[irq].isr_high = ((uint64_t)handler >> 32) & 0xFFFFFFFF;

    // GDT selector

    // Options
    idt[irq].options.ist = 0;
    idt[irq].options.reserved = 0;
    idt[irq].options.gate = 0; // Interrupt gate
    idt[irq].options.one = 0x7;
    idt[irq].options.zero = 0;
    idt[irq].options.dpl = 1; // ???
    idt[irq].options.present = 1;

    idt[irq].reserved = 0;
}