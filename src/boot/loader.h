#ifndef LOADER_H
#define LOADER_H

struct addr_range {
    intptr_t start;
    intptr_t end;
};

typedef struct loader_info {
    struct addr_range kernel;
    struct addr_range multiboot;
    intptr_t gdt64;
} loader_info_t;

#endif