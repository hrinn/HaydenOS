#ifndef PRINTK_H
#define PRINTK_H

#include <stdint-gcc.h>

int printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
int printb(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
uint64_t putc_sys_call(uint64_t);

#endif