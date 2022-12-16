#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint-gcc.h>

void *memset(void *dest, uint8_t c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *, const char *);
int strncmp(const char * s1, const char * s2, size_t n);
const char *strchr(const char *, int);
void strrev(char *);

#endif