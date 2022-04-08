#ifndef TABLE_REGISTER_H
#define TABLE_REGISTER_H

#include <stdint.h>

typedef struct {
    uint16_t limit;
    void *base;
} __attribute__((packed)) table_reg_t;

static inline void lidt(void *base, uint16_t size) {
    static table_reg_t idt_reg;

    idt_reg.base = base;
    idt_reg.limit = size;

    asm ( "lidt %0" : : "m"(idt_reg));
}

static inline void lgdt(void *base, uint16_t size) {
    static table_reg_t gdt_reg;

    gdt_reg.base = base;
    gdt_reg.limit = size;

    asm ( "lgdt %0" : : "m"(gdt_reg));
}

#endif