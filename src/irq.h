#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>

struct interrupt_frame;
typedef void (*irq_handler_t)(struct interrupt_frame, void *arg);

// ISR Numbers
#define DIVIDE_BY_ZERO 0
#define DEBUG 1
#define NON_MASKABLE_INTERRUPT 2
#define BREAKPOINT 3
#define OVERFLOW 4
#define BOUND_RANGE_EXCEEDED 5
#define INVALID_OPCODE 6
#define DEVICE_NOT_AVAILABLE 7
#define DOUBLE_FAULT 8
#define INVALID_TSS 10
#define SEGMENT_NOT_PRESENT 11
#define STACK_SEGMENT_FAULT 12
#define GENERAL_PROTECTION_FAULT 13
#define PAGE_FAULT 14
#define x87_FLOAT_POINT 16
#define ALIGNMENT_CHECK 17
#define MACHINE_CHECK 18
#define SIMD_FLOATING_POINT 19
#define VIRTUALIZATION 20
#define SECURITY_EXCEPTION 30

// IRQ Interface
void IRQ_init();
void IRQ_set_mask(int irq);
void IRQ_clear_mask(int irq);
int IRQ_get_mask(int IRQline);
void IRQ_end_of_interrupt(int irq);
void IRQ_set_handler(uint8_t irq, irq_handler_t handler, void *arg);

// Assembly helpers
static inline void cli() {
    asm volatile ("cli");
}

static inline void sti() {
    asm volatile ("sti");
}

#endif