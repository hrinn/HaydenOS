#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

void *memset(void *dest, uint8_t c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char *);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *, const char *);
const char *strchr(const char *, int);
// char *strdup(const char *);
void strrev(char *);

#endif