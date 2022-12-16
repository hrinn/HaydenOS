global long_mode_start, stack_bottom
extern kmain

section .multiboot.text exec    align=16
bits 64
long_mode_start:
    ; print 'OKAY' to screen
    mov rax, 0x2f472f4e2f4f2f4c
    mov qword [0xb8000], rax

    ; setup kernel stack
    mov rsp, stack_top

    ; load 0 into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
   
    call kmain

section .bss
align 0x1000
stack_bottom:
    resb 4096 * 2
stack_top: