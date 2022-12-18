global start, gdt64, p4_table
extern long_mode_start

section .multiboot.gdt64    align=4
gdt64:
    dq 0    ; null descriptor
.kernel_code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)    ; kernel mode code segment
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .multiboot.bss  nobits  write   align=4
align 0x1000
p4_table:
    resb 4096
p3_table_lower:
    resb 4096
p3_table_upper:
    resb 4096
mb_stack_bottom:
    resb 64
mb_stack_top:

section .multiboot.text exec    align=16
bits 32
start:
    ; disable interrupts
    cli

    ; print 'OK' to screen
    mov dword [0xb8000], 0x2f4b2f4f

    ; setup boot stack
    mov esp, mb_stack_top

    ; save multiboot information structure address
    mov edi, ebx

    ; perform checks
    call check_multiboot
    call check_cpuid
    call check_long_mode

    ; setup paging
    call setup_page_tables
    call enable_paging    

    ; load the 64-bit GDT
    lgdt [gdt64.pointer]
    jmp gdt64.kernel_code:long_mode_start
    
    hlt

; Prints 'ERR: ' and the given error code to screen and hangs.
; parameter: error code (in ascii) in al
error:
    mov dword [0xb8000], 0x4f524f45 ; 'ER'
    mov dword [0xb8004], 0x4f3a4f52 ; 'R:'
    mov dword [0xb8008], 0x4f204f20 ; '  '
    mov byte  [0xb800a], al         ; ASCII error byte
    hlt

; Checks if kernel was loaded by Multiboot bootloader by checking
; if the magic number is in eax
check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "0"
    jmp error

; Checks if CPUID is supported by attempting to flip the ID bit (bit 21)
; in the FLAGS register. If we can flip it, CPUID is available.
check_cpuid:
    pushfd              ; Copy FLAGS in to eax via stack
    pop eax             
    mov ecx, eax        ; Copy to ecx as well for comparing later on
    xor eax, 1 << 21    ; Flip the ID bit
    push eax            ; Copy eax to FLAGS via the stack
    popfd
    pushfd              ; Copy FLAGS back to eax (with the flipped bit if CPUID is supported)
    pop eax             
    push ecx            ; Restore FLAGS from old version stored in exc
    popfd
    cmp eax, ecx        ; Compare eax and exc. If they are equal then the bit wasn't flipped
    je .no_cpuid        ; and CPUID isn't supported
    ret
.no_cpuid:
    mov al, "1"
    jmp error

; Checks if the CPU booted in long mode by testing if extended processor
; info is available
check_long_mode:
    mov eax, 0x80000000     ; implicit argument for cpuid
    cpuid                   ; get highest supported argument
    cmp eax, 0x80000001     ; it needs to be at least 0x80000001
    jb .no_long_mode        ; if it's less, the CPU is too old for long mode

    ; use extended info to test if long mode is available
    mov eax, 0x80000001     ; argument for extended processor info
    cpuid                   ; returns various feature bits in ecx and edx
    test edx, 1 << 29       ; test if the LM-bit is set in the D-register
    jz .no_long_mode        ; if it's not set, there is no long mode
    ret
.no_long_mode:
    mov al, "2"
    jmp error

setup_page_tables:
    ; Setup P4 table
    mov eax, p3_table_lower    ; map first P4 entry to lower P3 table
    or eax, 0b11               ; present + writable
    mov [p4_table], eax

    mov eax, p3_table_upper    ; map last P4 entry to upper P4 table
    or eax, 0b11
    mov [p4_table + 511 * 8], eax

    ; Identity map first 2GB
    ; And map last 2GB to first 2GB
    ; Using 1GB huge pages in P3 table
    mov eax, 0x83       ; 0GB, present + writeable + huge
    mov [p3_table_lower], eax
    mov [p3_table_upper + 510 * 8], eax

    mov eax, 0x40000083 ; 1GB, present + writeable + huge
    mov [p3_table_lower + 8], eax
    mov [p3_table_upper + 511 * 8], eax

    ret

enable_paging:
    mov eax, p4_table       ; load physical address of P4 to cr3 register
    mov cr3, eax
    
    mov eax, cr4            ; enable PAE-flag in cr4 (Physical Address Extension)
    or eax, 1 << 5
    mov cr4, eax

    mov ecx, 0xC0000080     ; set the long mode bit in the EFER MSR (model specific register)
    rdmsr
    or eax, 1 << 8
    wrmsr

    mov eax, cr0            ; enable paging in the cr0 register
    or eax, 1 << 31
    mov cr0, eax

    ret