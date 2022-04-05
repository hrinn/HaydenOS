#include "irq.h"
#include <stdint.h>

// https://wiki.osdev.org/Interrupts_tutorial
typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t kernel_cs;     // The GDT segement selector that the CPU will load into CS before calling the ISR
    uint8_t ist;            // The IST in the TSS that teh CPU will load into RSP
    uint8_t attributes;     // Type and attributes
    uint16_t isr_mid;       // Middle 16 bits of ISR's address
    uint32_t isr_high;      // Upper 32 bits of ISR's address
    uint32_t reserved;      // 0
} __attribute__((packed)) idt_entry_t;

__attribute__((aligned(0x10)))
static idt_entry_t idt[256];

void IRQ_init() {
    // Register IDT
    // Configure PIC
}

// Mask determines which interrupts are enabled
void IRQ_set_mask(int irq) {

}

void IRQ_clear_mask(int irq) {

}

int IRQ_get_mask(int IRQline) {

}

void IRQ_end_of_interrupt(int irq) {

}

void IRQ_set_handler(int irq, irq_handler_t handler, void *arg) {
    idt_entry_t idt_entry = idt[irq];

}