global check_int
global load_cr3

section .text
bits 64
check_int:
    pushf           ; push the flags register to the stack
    mov ax, [rsp]   ; move this value into ax
    popf            ; pop the flags register from the stack
    and ax, 0x200   ; mask out interrupt flag
    ret

; load_cr3:
;     mov cr3, rdi
;     ret