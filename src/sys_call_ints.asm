section .text
bits 64

%macro VOID_SYSCALL_VOID 1
    push rax
    push rdi
    push rsi
    mov rdi, %1
    mov rsi, 0
    int 206
    pop rsi
    pop rdi
    pop rax
    ret
%endmacro

%macro RET_SYSCALL_VOID 1
    push rdi
    push rsi
    mov rdi, %1  ; interrupt
    mov rsi, 0  ; argument
    int 206
    pop rsi
    pop rdi
    ret
%endmacro

%macro VOID_SYSCALL_ARG 1
    push rax
    push rsi
    mov rsi, rdi    ; argument
    mov rdi, %1      ; interrupt
    int 206
    pop rsi
    pop rax
    ret
%endmacro

global yield
yield:
    VOID_SYSCALL_VOID 0

global kexit
kexit:
    VOID_SYSCALL_VOID 1

global getc
getc:
    RET_SYSCALL_VOID 2

global putc
putc:
    VOID_SYSCALL_ARG 3
