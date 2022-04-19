#ifndef MMU_H
#define MMU_H

#include <stdint-gcc.h>

#define PAGE_SIZE 4096

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

void parse_multiboot_tags(struct multiboot_info *);
void *MMU_pf_alloc(void);
void MMU_pf_free(void *pf);

#endif