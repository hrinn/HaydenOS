global check_int
global enable_no_execute
global call_user

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

; Jumps into user space
; rdi: Address of user space program
; rsi: Address of user space stack top
call_user:
    ; Setup stack expected by iretq
    sub rsp, 32
    mov qword [rsp], rdi  ; RIP
    mov qword [rsp + 8], 0x18 | 0x3   ; User Code Selector | User DPL
    mov qword [rsp + 16], 0x202      ; RFLAGS (IOPL = 3, IE = 1)
    mov qword [rsp + 24], rsi         ; RSP
    mov qword [rsp + 32], 0x20 | 0x3  ; User Stack Selector | User DPL

    iretq
