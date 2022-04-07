#include "isr.h"
#include "printk.h"

#define UNUSED __attribute__((unused))


void handler(UNUSED struct interrupt_frame *frame) {
    cli();
    printk("interrupt\n");
    sti();
}

void err_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("interrupt with error code: 0x%lx\n", error_code);
    sti();
}

void double_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("double fault 0x%lx\n", error_code);
    sti();
}

void invalid_tss_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("invalid tss 0x%lx\n", error_code);
    sti();
}

void segment_not_present_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("segment not present 0x%lx\n", error_code);
    sti();
}

void page_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("page_fault 0x%lx\n", error_code);
    sti();
}

void control_protection_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("control protection 0x%lx\n", error_code);
    sti();
}

void stack_segment_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("stack segment 0x%lx\n", error_code);
    sti();
}

void general_protection_fault_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("general protection fault 0x%lx\n", error_code);
    sti();
}

void alignment_check_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("alignment check 0x%lx\n", error_code);
    sti();
}

void security_exception_handler(UNUSED struct interrupt_frame *frame, unsigned long int error_code) {
    cli();
    printk("security exception 0x%lx\n", error_code);
    sti();
}