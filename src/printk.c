#include "printk.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "vga.h"
#include "string.h"

#define FORMAT_BUFF 20

struct format_info {
    uint8_t length;
    char length_specifier;
    char format_specifier;
};

// Converts a number to a string, places result in buffer
// Returns number of written digits including -
void itoa(int num, char *buffer, int base, bool caps) {
    int i = 0, rem;
    bool negative = false;
    char hexstart = (caps) ? 'A' : 'a';

    if (buffer == NULL) {
        return;
    }

    // Handle 0
    if (num == 0) {
        buffer[i] = '0';
        buffer[++i] = '\0';
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
        buffer[i++] = (rem < 10) ? rem + '0' : (rem - 10) + hexstart;
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

// Converts an unsigned number to a string, places result in buffer
// Returns number of written digits including -
void uitoa(unsigned int num, char *buffer, int base, bool caps) {
    int i = 0, rem;
    char hexstart = (caps) ? 'A' : 'a';

    if (buffer == NULL) {
        return;
    }

    // Handle 0
    if (num == 0) {
        buffer[i] = '0';
        buffer[++i] = '\0';
        return;
    }

    // Convert number to string
    while (num > 0) {
        rem = num % base;
        buffer[i++] = (rem < 10) ? rem + '0' : (rem - 10) + hexstart;
        num /= base;
    }

    // Null terminate
    buffer[i] = '\0';

    // Reverse string
    strrev(buffer);
}

static inline void print_uchar(unsigned char u) {
    VGA_display_char(u);
}

static inline void print_char(char c) {
    VGA_display_char(c);
}

static inline void print_str(const char *s) {
    VGA_display_str(s);
}

static inline void print_strn(const char *s, int n) {
    VGA_display_strn(s, n);
}

void print_int(int i) {
    char buffer[FORMAT_BUFF];
    itoa(i, buffer, 10, false);
    VGA_display_str(buffer);
}

void print_uint(unsigned int u) {
    char buffer[FORMAT_BUFF];
    uitoa(u, buffer, 10, false);
    VGA_display_str(buffer);
}

void print_hex(unsigned int h, bool caps) {
    char buffer[FORMAT_BUFF];
    uitoa(h, buffer, 16, caps);
    VGA_display_str(buffer);
}

void print_pointer(unsigned int p) {
    char buffer[FORMAT_BUFF];
    buffer[0] = '0';
    buffer[1] = 'x';
    uitoa(p, buffer + 2, 16, false);
    VGA_display_str(buffer);
}

// Takes a format string, pointing to the % character
// Fills the format info struct
// Returns true if it is a valid specifier, false if invalid
bool check_format_spec(char *fmt, struct format_info *info, int rem_len) {

    if (*fmt != '%') return false;

    // Ensure there is enough remaining length

    // Check the length specifier
    switch (*(++fmt)) {
        case 'h':
        case 'l':
        case 'q':
            info->length_specifier = *fmt;
            break;
        default:
            break;
    }

    // Check the format specifier
    switch (*(++fmt)) {
        case '%':
        case 'd':
        case 'u':
        case 'x':
        case 'X':
        case 'c':
        case 'p':
        case 's':
            info->format_specifier = *fmt;
            break;
        default:
            return false;
            break;
    }

    // Set length
    return true;
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
        // Check length and validity of format specifier
        if (fmt[j] == '%' && j < len - 1) {
            format = true;
            split = true;
        } else if (fmt[j] == '\0') {
            split = true;
        }

        if (split) {
            print_strn(fmt + i, j - i);
        }

        if (format) {
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
                    print_hex(va_arg(valist, unsigned int), false);
                    break;
                case 'X': // Uppercase hex
                    print_hex(va_arg(valist, unsigned int), true);
                    break;
                case 'c': // char
                    print_char((char)va_arg(valist, int));
                    break;
                case 'p': // Pointer address
                    print_pointer(va_arg(valist, unsigned int));
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
            i = ++j + 1;
        }
        format = false;
        split = false;
    } while (fmt[j++]);

    va_end(valist);

    return 1;
}