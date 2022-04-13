#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

typedef void (*irq_handler_t)(int, int, void *);

// IRQ Interface
void IRQ_init();
void IRQ_set_mask(uint8_t irq);
void IRQ_clear_mask(uint8_t irq);
int IRQ_get_mask(int IRQline);
void IRQ_end_of_interrupt(int irq);
void IRQ_set_handler(uint8_t irq, irq_handler_t handler);

// Assembly helpers
static inline void cli() {
    asm volatile ("cli");
}

static inline void sti() {
    asm volatile ("sti");
}

#endif