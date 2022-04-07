#include "irq.h"

__attribute__((interrupt)) void handler(struct interrupt_frame *frame);
__attribute__((interrupt)) void err_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void double_fault_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void invalid_tss_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void segment_not_present_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void page_fault_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void control_protection_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void stack_segment_fault_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void general_protection_fault_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void alignment_check_handler(struct interrupt_frame *frame, unsigned long int error_code);
__attribute__((interrupt)) void security_exception_handler(struct interrupt_frame *frame, unsigned long int error_code);
