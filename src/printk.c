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
void itoa(int64_t num, char *buffer, int base, bool caps) {
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
void uitoa(uint64_t num, char *buffer, int base, bool caps) {
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

void print_int(int64_t i) {
    char buffer[FORMAT_BUFF];
    itoa(i, buffer, 10, false);
    VGA_display_str(buffer);
}

void print_uint(uint64_t u) {
    char buffer[FORMAT_BUFF];
    uitoa(u, buffer, 10, false);
    VGA_display_str(buffer);
}

void print_hex(uint64_t h, bool caps) {
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
bool check_format_spec(const char *fmt, struct format_info *info) {
    if (*fmt != '%') return false;

    // Check the length specifier
    switch (*(fmt + 1)) {
        case 'h':
        case 'l':
        case 'q':
            info->length_specifier = *(++fmt);
            break;
        default:
            break;
    }

    // Check the format specifier supports length specifiers
    if (info->length_specifier) {
        switch (*(fmt + 1)) {
            case 'd':
            case 'u':
            case 'x':
            case 'X':
                info->length = 3;
                info->format_specifier = *(fmt + 1);
                return true;
            default:
                return false;
        }
    }

    // Check the format specifier
    switch (*(fmt + 1)) {
        case '%':
        case 'd':
        case 'u':
        case 'x':
        case 'X':
        case 'c':
        case 'p':
        case 's':
            info->length = 2;
            info->format_specifier = *(fmt + 1);
            return true;
        default:
            return false;
    }
}

// Like printf, but in the kernel
int printk(const char *fmt, ...) {
    va_list valist;
    int i = 0, j = 0;
    bool split, format;
    struct format_info info;

    if (fmt == NULL) return -1;
    va_start(valist, fmt);

    do {
        info.length = 0;
        info.format_specifier = '\0';
        info.length_specifier = '\0';
        split = false;
        format = false;

        // Determine action on this iteration
        // Check length and validity of format specifier
        if (fmt[j] == '\0') {
            split = true;
        } else if (check_format_spec(fmt + j, &info)) {
            split = true;
            format = true;
        }

        if (split) {
            print_strn(fmt + i, j - i);
        }

        if (format) {
            switch (info.format_specifier) {
                case '%': // Percent
                    print_char('%');
                    break;
                case 'd': // Signed integer
                    print_int(va_arg(valist, int64_t));
                    break;
                case 'u': // Unsigned integer
                    print_uint(va_arg(valist, uint64_t));
                    break;
                case 'x': // Lowercase hex
                    print_hex(va_arg(valist, uint64_t), false);
                    break;
                case 'X': // Uppercase hex
                    print_hex(va_arg(valist, uint64_t), true);
                    break;
                case 'c': // Char
                    print_char((char)va_arg(valist, int));
                    break;
                case 'p': // Pointer address
                    print_pointer(va_arg(valist, unsigned int));
                    break;
                case 's': // String
                    print_str(va_arg(valist, char *));
                    break;
                default: // Invalid specifier
                    return -1;
            }
            // Move iterators to next substring
            j += info.length;
            i = j--; // Decrement j because the while loop performs increment
        }
    } while (fmt[j++]);

    va_end(valist);

    return 1;
}