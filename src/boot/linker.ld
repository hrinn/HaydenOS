ENTRY(start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    . = 1M;

    .boot : {
        *(.multiboot_header)
    }

    .rodata : {
        *(.rodata)
    }

    .text : {
        *(.text)
    }

    .bss : {
        *(.bss)
    }
}