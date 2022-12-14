global long_mode_start, stack_bottom, p4_table
extern kmain

KERNEL_VBASE equ 0xFFFFFFFF80000000

section .multiboot.text exec    align=16
bits 64
long_mode_start:
    ; print 'OKAY' to screen
    mov rax, 0x2f472f4e2f4f2f4c
    mov qword [0xb8000], rax

    ; map last 2GB of address space to first 2GB
    call setup_higher_half_map

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

setup_higher_half_map:
    ; Map last 2GB to first 2GB
    mov rax, 0x83       ; 0GB, present + writeable + huge
    mov rbx, (p3_table + 510 * 8) - KERNEL_VBASE
    mov [rbx], rax

    mov rax, 0x40000083 ; 1GB, present + writeable + huge
    mov rbx, (p3_table + 511 * 8) - KERNEL_VBASE
    mov [rbx], rax

    mov rax, (p3_table - KERNEL_VBASE)  ; map last P4 entry to last P3 table
    or rax, 0b11
    mov rbx, (p4_table + 511 * 8) - KERNEL_VBASE
    mov [rbx], rax

    ret

section .bss
align 0x1000
p4_table:
    resb 4096
p3_table:
    resb 4096
stack_bottom:
    resb 4096 * 2
stack_top: