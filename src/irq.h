#ifndef IRQ_H
#define IRQ_H

#include <stdint-gcc.h>
#include "registers.h"

#define CLI asm volatile ("cli")
#define STI asm volatile ("sti")

typedef void (*irq_handler_t)(uint8_t irq, uint32_t error_code, void *arg);

// IRQ Interface
void IRQ_init();
void IRQ_set_handler(uint8_t irq, irq_handler_t handler, void *arg);

// PIC Interface
void IRQ_set_mask(uint8_t irq);
void IRQ_clear_mask(uint8_t irq);
uint8_t IRQ_get_mask(uint8_t irq);
void IRQ_end_of_interrupt(uint8_t irq);

// Assembly helpers
extern uint16_t check_int(void);

void apply_isr_offset(uint64_t offset);

static inline void cli() {
    asm volatile ("cli");
}

static inline void sti() {
    asm volatile ("sti");
}

#endif