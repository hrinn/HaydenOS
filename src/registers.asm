global check_int
global enable_no_execute

section .text
bits 64
check_int:
    pushf           ; push the flags register to the stack
    mov ax, [rsp]   ; move this value into ax
    popf            ; pop the flags register from the stack
    and ax, 0x200   ; mask out interrupt flag
    ret

enable_no_execute:
    mov rcx, 0xC0000080
    rdmsr
    or rax, 1 << 11
    wrmsr
    ret