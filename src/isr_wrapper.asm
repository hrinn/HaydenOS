extern irq_handler_table

; isr macro
%macro ISR_MACRO 1
global isr_wrapper_%1
isr_wrapper_%1:
    ; push scratch registers to the stack
    push rax
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    ; place IRQ number in rdi
    mov rdi, %1

    ; place error code in rsi
    mov rsi, [rsp + 64]

    ; call C function
    call [irq_handler_table + (%1 * 8)]

    ; restore registers
    pop rax
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    iretq
%endmacro

section .text
bits 64
ISR_MACRO 0
ISR_MACRO 1
ISR_MACRO 2
ISR_MACRO 3
ISR_MACRO 4
ISR_MACRO 5
ISR_MACRO 6
ISR_MACRO 7
ISR_MACRO 8
ISR_MACRO 9
ISR_MACRO 10
ISR_MACRO 11
ISR_MACRO 12
ISR_MACRO 13
ISR_MACRO 14
ISR_MACRO 15
ISR_MACRO 16
ISR_MACRO 17
ISR_MACRO 18
ISR_MACRO 19
ISR_MACRO 20
ISR_MACRO 21
ISR_MACRO 22
ISR_MACRO 23
ISR_MACRO 24
ISR_MACRO 25
ISR_MACRO 26
ISR_MACRO 27
ISR_MACRO 28
ISR_MACRO 29
ISR_MACRO 30
ISR_MACRO 31