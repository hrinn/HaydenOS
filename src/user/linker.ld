ENTRY(main)

SECTIONS {
    . = 0x8000000000;
    
    .text : {
        *(.text);
    }

    .data : {
        *(.data);
    }

    .bss : {
        *(.bss);
    }

    .rodata : {
        *(.rodata);
    }
}