#include <stdint-gcc.h>
#include <stddef.h>
#include "multiboot.h"
#include "loader.h"

void loader(struct multiboot_info *multiboot_info) {
    int pause = 1;
    while (pause);

    loader_info_t loader_info;
    parse_multiboot_tags(multiboot_info, &loader_info);

    while (1) asm ("hlt");
}