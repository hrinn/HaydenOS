global stack_top, stack_bottom, ist1_top, ist1_bottom, ist2_top, ist2_bottom, ist3_top, ist3_bottom

section .bss
stack_bottom:
    resb 4096 * 2
stack_top:

ist1_bottom:
    resb 4096
ist1_top:

ist2_bottom:
    resb 4096
ist2_top:

ist3_bottom:
    resb 4096
ist3_top: