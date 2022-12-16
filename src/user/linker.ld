ENTRY(main)

SECTIONS {
    . = 0x8000000000;
    
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