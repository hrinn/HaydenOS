#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint-gcc.h>

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

static inline void ltr(uint16_t offset) {
    asm ( "ltr %0" : : "m"(offset));
}

static inline uint64_t get_cr3() {
    uint64_t res;
    asm ( "movq %%cr3, %0" : "=r"(res));
    return res;
}

static inline void set_cr3(uint64_t data) {
    asm ( "movq %0, %%cr3" : : "r"(data));
}

static inline uint64_t get_cr2() {
    uint64_t res;
    asm ( "movq %%cr2, %0" : "=r"(res));
    return res;
}

#endif