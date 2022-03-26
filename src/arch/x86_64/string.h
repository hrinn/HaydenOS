#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void *memset(void *, int, size_t);
void *memcpy(void *, const void *, size_t);
size_t strlen(const char *);
char *strcpy(char *, const char *);
char *strncpy(char *, const char *, size_t);
int strcmp(const char *, const char *);
const char *strchr(const char *, int);
char *strdup(const char *);
void strrev(char *);

#endif