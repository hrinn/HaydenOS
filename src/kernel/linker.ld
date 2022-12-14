ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

_virtual_base = 0xFFFFFFFF80000000;

SECTIONS {
    . = 1M;

    .multiboot : 
    {
        *(.multiboot.header)
        *(.multiboot.gdt64)
        *(.multiboot.text)
        *(.multiboot.bss)
    }

    . += _virtual_base;

    .text ALIGN(4K) : AT(ADDR(.text) - _virtual_base)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - _virtual_base)
    {
        *(.rodata)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - _virtual_base)
    {
        *(.data)
    }

    .bss ALIGN(4K) : AT(ADDR(.bss) - _virtual_base)
    {
        *(.bss)
    }
}