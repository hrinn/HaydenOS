#include "printk.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include "vga.h"
#include "string.h"

#define FORMAT_BUFF 20

// Converts a number to a string, places result in buffer
// Returns number of written digits including -
void itoa(int num, char *buffer, int base) {
    int i = 0, rem;
    bool negative = false;

    if (buffer == NULL) {
        return;
    }

    // Handle 0
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Only treat base 10 numbers as signed
    if (base == 10 && num < 0) {
        negative = true;
        num *= -1;
    }

    // Convert number to string
    while (num > 0) {
        rem = num % base;
        buffer[i++] = (rem < 10) ? rem + '0' : (rem - 10) + 'a';
        num /= base;
    }

    // Append '-'
    if (negative) {
        buffer[i++] = '-';
    }

    // Null terminate
    buffer[i] = '\0';

    // Reverse string
    strrev(buffer);
}

// Converts a base 10 unsigned number to a string, places result in buffer
// Returns number of written digits including -
void uitoa(unsigned int num, char *buffer) {
    int i = 0;

    if (buffer == NULL) {
        return;
    }

    // Handle 0
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return;
    }

    // Convert number to string
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
    }

    // Null terminate
    buffer[i] = '\0';

    // Reverse string
    strrev(buffer);
}

void print_uchar(unsigned char u) {
    VGA_display_char(u);
}

void print_char(char c) {
    VGA_display_char(c);
}

void print_str(const char *s) {
    VGA_display_str(s);
}

void print_strn(const char *s, int n) {
    char buffer[n];
    strncpy(buffer, s, n);
    VGA_display_str(buffer);
}

void print_int(int i) {
    char buffer[FORMAT_BUFF];
    itoa(i, buffer, 10);
    VGA_display_str(buffer);
}

void print_uint(unsigned int u) {
    char buffer[FORMAT_BUFF];
    uitoa(u, buffer);
    VGA_display_str(buffer);
}

void print_hex(int h) {
    char buffer[FORMAT_BUFF];
    itoa(h, buffer, 16);
    VGA_display_str(buffer);
}

void print_pointer(int p) {
    char buffer[FORMAT_BUFF];
    buffer[0] = '0';
    buffer[1] = 'x';
    itoa(p, buffer + 2, 16);
    VGA_display_str(buffer);
}

// Like printf, but in the kernel
int printk(const char *fmt, ...) {
    va_list valist;
    int len, i = 0, j = 0;
    bool split = false, format = false;

    if (fmt == NULL) return -1;
    len = strlen(fmt);
    va_start(valist, fmt);

    do {
        // Determine action on this iteration
        if (fmt[j] == '%' && j < len - 1) {
            format = true;
            split = true;
        } else if (fmt[j] == '\0') {
            split = true;
        }

        if (split) {
            // Print fmt[i:j]
            print_strn(fmt + i, j - i);
            i = j;
        }

        if (format) {
            // Print arg with specified formatting
            switch (fmt[j + 1]) {
                case '%': // Percent
                    print_char('%');
                    break;
                case 'd': // Signed integer
                    print_int(va_arg(valist, int));
                    break;
                case 'u': // Unsigned integer
                    print_uint(va_arg(valist, unsigned int));
                    break;
                case 'x': // Lowercase hex
                    print_hex(va_arg(valist, int));
                    break;
                case 'c': // char
                    print_char((char)va_arg(valist, int));
                    break;
                case 'p': // Pointer address
                    print_pointer(va_arg(valist, int));
                    break;
                case 'h': // h[dux], short length specifier
                    break;
                case 'l': // l[dux], long length specifier
                    break;
                case 'q': // q[dux], ???
                    break;
                case 's':
                    print_str(va_arg(valist, char *));
                    break;
                default: // Invalid specifier
                    return -1;
            }
            // Move iterators past index character
            i++;
            j++;
        }
        format = false;
        split = false;
    } while (fmt[j++]);

    va_end(valist);
}