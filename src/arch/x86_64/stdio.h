#ifndef STDIO_H
#define STDIO_H

int printk(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

#endif