global long_mode_start
extern loader

section .text
bits 64
long_mode_start:
    ; load 0 intal all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; print 'OKAY' to screen
    mov rax, 0x2f472f4e2f4f2f4c
    mov qword [0xb8000], rax

    ; mov the tag structure pointer into edi
    ; mov edi, ebx

    call loader