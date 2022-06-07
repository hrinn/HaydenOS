ENTRY(kmain)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS {
    . = 0xffff800000000000;

    .rodata :
    {
        *(.rodata .rodata.*)
        . = ALIGN(4K);
    }

    .eh_frame :
    {
        *(.eh_frame .eh_frame.*)
        . = ALIGN(4K);
    }

    .text :
    {
        *(.text .text.*)
        . = ALIGN(4K);
    }

    .data :
    {
        *(.data .data.*)
        . = ALIGN(4K);
    }

    .bss :
    {
        *(.bss .bss.*)
        . = ALIGN(4K);
    }
}