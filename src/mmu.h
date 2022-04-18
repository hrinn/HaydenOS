#ifndef MMU_H
#define MMU_H

#include <stdint.h>

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

void parse_multiboot_tags(struct multiboot_info *);

#endif