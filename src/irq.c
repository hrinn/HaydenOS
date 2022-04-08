#include "irq.h"
#include "printk.h"
#include "table_register.h"

#define FLAGS 0x8E // 1000 1110 
#define NUM_ENTRIES 256
#define NUM_HANDLERS 12
#define NUM_ERR_HANDLERS 9
#define UNUSED __attribute__((unused))

typedef struct {
    uint16_t isr_low;       // Lower 16 bits of ISR's address
    uint16_t gdt_selector;  // Selects a code segment in the GDT
    uint8_t ist;            // IST in the TSS that the CPU will load into RSP
    uint8_t attributes;     // Type and attributes
    uint16_t isr_mid;       // Middle 16 bits of ISR's address
    uint32_t isr_high;      // Upper 32 bits of ISR's address
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

// Interrupt descriptor table
__attribute__((aligned(16)))
static idt_entry_t idt[NUM_ENTRIES];

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

__attribute__((interrupt))
void double_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("double fault 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void invalid_tss_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("invalid tss 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void segment_not_present_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("segment not present 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void page_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("page_fault 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void control_protection_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("control protection 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void stack_segment_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("stack segment 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void general_protection_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("general protection fault 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void alignment_check_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("alignment check 0x%lx\n", error_code);
    sti();
}

__attribute__((interrupt))
void security_exception_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("security exception 0x%lx\n", error_code);
    sti();
}

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

    // Load IDT register
    lidt(&idt[0], sizeof(idt_entry_t) * NUM_ENTRIES - 1);

    // Set interrupt mask

    // Set interrupt flag
    // sti();
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