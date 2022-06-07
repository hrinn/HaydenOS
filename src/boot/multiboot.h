#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint-gcc.h>

struct multiboot_info {
    uint32_t total_size;
    uint32_t reserved;
};

typedef struct loader_info loader_info_t;
void parse_multiboot_tags(struct multiboot_info *multiboot_tags, loader_info_t *loader_info);

#endif