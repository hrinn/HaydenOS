#include "string.h"

void *memset(void *dst, int c, size_t n) {
    int i;
    char *s = (char *)dst;
    
    if (dst == NULL) return NULL;

    for (i = 0; i < n; i++) {
        s[i] = c;
    }

    return dst;
}

void *memcpy(void *dest, const void *src, size_t n) {
    int i;

    char *d = (char *)dest;
    char *s = (char *)src;

    if (dest == NULL) return NULL;
    if (src == NULL) return dest;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

size_t strlen(const char *s) {
    size_t len = 0;
    if (s == NULL) return 0;
    while (s[len]) len++;
    return len;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    if (dest == NULL) return NULL;
    if (src == NULL) return dest;

    do {
        dest[i] = src[i];
        i++;
    } while (src[i] != '\0');

    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    if (dest == NULL) return NULL;
    if (src == NULL) return dest;

    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for( ; i < n; i++) {
        dest[i] = '\0';
    }

    return dest;
}

int strcmp(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) return -1;

    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

const char *strchr(const char *s, int c) {
    if (s == NULL) return NULL;

    while (*s) {
        if (*s == c) {
            return s;
        }
        s++;
    }
    return NULL;
}

/* Not implemented, no malloc on system */
char *strdup(const char *s) {
    return NULL;
}

// Reverses a string in place
void strrev(char *str) {
    int i = 0, j = strlen(str) - 1;
    char t;

    while (i < j) {
        t = str[i];
        str[i++] = str[j];
        str[j--] = t;
    }
}