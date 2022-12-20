#include "test.h"
#include "heap_alloc.h"
#include "stddef.h"
#include "stdbool.h"
#include "printk.h"
#include "kmalloc.h"
#include "snakes.h"
#include "proc.h"

void write_uniq(void *addr, size_t len) {
    uint8_t data = ((uint64_t)addr) & 0xFF;
    size_t i;
    uint8_t *writeable = (uint8_t *)addr;

    for (i = 0; i < len; i++) {
        writeable[i] = data;
    }
}

bool check_uniq(void * addr, size_t len) {
    uint8_t data = ((uint64_t)addr) & 0xFF;
    size_t i;
    uint8_t *readable = (uint8_t *)addr;

    for (i = 0; i < len; i++) {
        if (readable[i] != data) {
            return false;
        }
    }

    return true;
}

void test_page_alloc() {
    void *page;

    while(1) {
        page = MMU_alloc_page();
        write_uniq(page, PAGE_SIZE);
        if (!check_uniq(page, PAGE_SIZE)) {
            printk("Failed to validate page %p\n", page);
        }
    }
}

void test_kmalloc() {
    void *addr;
    int n;

    while (1) {
        for (n = 32; n < 4096; n *= 2) {
            addr = kmalloc(n);
            write_uniq(addr, n);
            if (!check_uniq(addr, n)) {
                printk("Failed to validate allocated memory %p\n", addr);
            }
        }
    }
}

void test_snakes() {
    setup_snakes(1);
    PROC_run();
    printk("Back to kmain!\n");
}