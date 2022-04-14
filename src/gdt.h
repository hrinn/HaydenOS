#ifndef GDT_H
#define GDT_H

void GDT_remap(void);
void TSS_init(void);

// Offsets
#define KERNEL_CODE_OFFSET 0x8

#endif