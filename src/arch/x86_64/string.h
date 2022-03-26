#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
size_t strlen(const char *);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *, const char *);
const char *strchr(const char *, int);
char *strdup(const char *);
void strrev(char *);

#endif