#ifndef SERIAL_H
#define SERIAL_H
#include <stdint-gcc.h>

int SER_init(void);
int SER_write(const char *buff, int len);
int SER_writeb(const char *buff, int len);

#endif