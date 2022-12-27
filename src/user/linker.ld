ENTRY(main)

SECTIONS {
    . = 0x1M;
    
    .text : {
        *(.text);
    }

    .rodata : {
        *(.rodata);
    }

    .data : {
        *(.data);
    }

    .bss : {
        *(.bss);
    }
}