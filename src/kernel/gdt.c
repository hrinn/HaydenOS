#include "gdt.h"
#include <stdint-gcc.h>
#include "string.h"
#include "registers.h"

#define GDT_LIMIT 55
#define TSS_SELECTOR 0x28
#define TSS_TYPE 0x9

extern uint64_t gdt64;

typedef struct {
    uint16_t limit_15_0;
    uint16_t addr_15_0;
    uint8_t addr_23_16;

    uint8_t type : 4;
    uint8_t zero : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;

    uint8_t limit_19_16 : 4;
    uint8_t avl : 1;
    uint8_t res1 : 2;
    uint8_t g : 1;

    uint8_t addr_31_24;
    uint32_t addr_63_32;
    uint32_t res2;
} __attribute__((packed)) tss_descriptor_t;

typedef struct {
    uint32_t res1;
    uint64_t rsp[3];
    uint64_t res2;
    uint64_t ist[7];
    uint64_t res3;
    uint16_t res4;
    uint16_t io_map_base_addr;
} __attribute__((packed)) tss_t;

typedef struct {
    uint16_t limit_15_0;
    uint16_t addr_15_0;
    uint8_t addr_23_16;

    uint8_t a : 1;
    uint8_t r : 1;
    uint8_t c : 1;
    uint8_t one1 : 1;
    uint8_t one2 : 1;
    uint8_t dpl : 2;
    uint8_t present : 1;

    uint8_t limit_19_16 : 4;
    uint8_t avl : 1;
    uint8_t l : 1;
    uint8_t d : 1;
    uint8_t g : 1;

    uint8_t addr_31_24;
} __attribute__((packed)) code_descriptor_t;

typedef struct {
    uint64_t null_descriptor;
    code_descriptor_t kernel_code_descriptor;
    uint64_t kernel_data_descriptor;    // Currently unused
    code_descriptor_t user_code_descriptor;
    uint64_t user_data_descriptor;
    tss_descriptor_t tss_descriptor;
} __attribute__((packed)) gdt_t;

static tss_t tss;
static gdt_t gdt;
extern uint8_t ist_stack1_top;
extern uint8_t ist_stack2_top;
extern uint8_t ist_stack3_top;

void GDT_remap() {
    // Copy GDT 64 defined in boot.asm to new gdt
    memcpy(&gdt, &gdt64, 16);

    // Setup user descriptors
    gdt.user_code_descriptor.one1 = 1;
    gdt.user_code_descriptor.one2 = 1;
    gdt.user_code_descriptor.dpl = USER_DPL;
    gdt.user_code_descriptor.present = 1;
    gdt.user_code_descriptor.l = 1;

    // Load new GDT into GDT_R
    lgdt(&gdt, GDT_LIMIT);
}

void TSS_init() {
    uint32_t tss_limit = sizeof(tss) - 1;
    uint64_t tss_addr = (uint64_t)&tss;

    // Initialize TSS
    tss.ist[0] = (uint64_t)&ist_stack1_top;
    tss.ist[1] = (uint64_t)&ist_stack2_top;
    tss.ist[2] = (uint64_t)&ist_stack3_top;

    // Initialize TSS descriptor
    gdt.tss_descriptor.limit_15_0 = tss_limit & 0xFFFF;
    gdt.tss_descriptor.limit_19_16 = (tss_limit >> 16) & 0xF;

    gdt.tss_descriptor.addr_15_0 = tss_addr & 0xFFFF;
    gdt.tss_descriptor.addr_23_16 = (tss_addr >> 16) & 0xFF;
    gdt.tss_descriptor.addr_31_24 = (tss_addr >> 24) & 0xFF;
    gdt.tss_descriptor.addr_63_32 = (tss_addr >> 32) & 0xFFFFFFFF;

    gdt.tss_descriptor.type = TSS_TYPE;
    gdt.tss_descriptor.zero = 0;
    gdt.tss_descriptor.present = 1;
    gdt.tss_descriptor.avl = 0;
    gdt.tss_descriptor.res1 = 0;
    gdt.tss_descriptor.g = 0;
    gdt.tss_descriptor.res2 = 0;

    // Load the offset of the TSS descriptor in the GDT
    ltr(TSS_SELECTOR);
}

void TSS_remap(virtual_addr_t *stack_tops, int n) {
    int i;
    for (i = 0; i < n; i++) {
        tss.ist[i] = stack_tops[i];
    }
}

void TSS_set_ist(virtual_addr_t stack_top, int ist) {
    tss.ist[ist - 1] = stack_top;
}