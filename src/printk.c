#include "printk.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdint-gcc.h>
#include <stdbool.h>
#include "vga.h"
#include "string.h"
#include "serial.h"

#define FORMAT_BUFF 20

struct format_info {
    uint8_t length;
    char length_specifier;
    char format_specifier;
};

// Converts a number to a string, places result in buffer
// Returns number of written digits including -
static void itoa(int64_t num, char *buffer, int base, bool caps) {
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
static void uitoa(uint64_t num, char *buffer, int base, bool caps) {
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

static inline void print(const char *buff, int len) {
    VGA_display_str(buff, len);
    SER_write(buff, len);
}

static inline void print_uchar(unsigned char u) {
    print((char *)&u, 1);
}

static inline void print_char(char c) {
    print(&c, 1);
}

static inline void print_str(const char *s) {
    print(s, strlen(s));
}

static inline void print_strn(const char *s, int len) {
    print(s, len);
}

static void print_int(int64_t i) {
    char buffer[FORMAT_BUFF];
    itoa(i, buffer, 10, false);
    print(buffer, FORMAT_BUFF);
}

static void print_uint(uint64_t u) {
    char buffer[FORMAT_BUFF];
    uitoa(u, buffer, 10, false);
    print(buffer, FORMAT_BUFF);
}

static void print_hex(uint64_t h, bool caps) {
    char buffer[FORMAT_BUFF];
    uitoa(h, buffer, 16, caps);
    print(buffer, FORMAT_BUFF);
}


static void print_pointer(uint64_t p) {
    char buffer[FORMAT_BUFF];
    buffer[0] = '0';
    buffer[1] = 'x';
    uitoa(p, buffer + 2, 16, false);
    print(buffer, FORMAT_BUFF);
}

// Takes a format string, pointing to the % character
// Fills the format info struct
// Returns true if it is a valid specifier, false if invalid
static bool check_format_spec(const char *fmt, struct format_info *info) {
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
    int start = 0, end = 0;
    struct format_info info;
    bool format_flag, end_flag;

    if (fmt == NULL) return -1;
    va_start(valist, fmt);

    do {
        info.length = 0;
        info.format_specifier = '\0';
        info.length_specifier = '\0';
        format_flag = false;
        end_flag = false;

        end_flag = (fmt[end] == '\0');
        format_flag = check_format_spec(fmt + end, &info);

        if (end_flag || format_flag) {
            print_strn(fmt + start, end - start);
        }

        if (format_flag) {
            switch (info.format_specifier) {
                case '%': // Percent
                    print_char('%');
                    break;
                case 'd': // Signed integer
                    switch (info.length_specifier) {
                        case 'h': print_int((int16_t)va_arg(valist, int32_t));          break;
                        case 'l': print_int(va_arg(valist, int64_t));                   break;
                        case 'q': print_int(va_arg(valist, int64_t));                   break;
                        case '\0': print_int(va_arg(valist, int32_t));                  break;
                    }     
                    break;
                case 'u': // Unsigned integer
                    switch (info.length_specifier) {
                        case 'h': print_uint((uint16_t)va_arg(valist, uint32_t));       break;
                        case 'l': print_uint(va_arg(valist, uint64_t));                 break;
                        case 'q': print_uint(va_arg(valist, uint64_t));                 break;
                        case '\0': print_uint(va_arg(valist, uint32_t));                break;
                    }
                    break;
                case 'x': // Lowercase hex
                    switch (info.length_specifier) {
                        case 'h': print_hex((uint16_t)va_arg(valist, uint32_t), false); break;
                        case 'l': print_hex(va_arg(valist, uint64_t), false);           break;
                        case 'q': print_hex(va_arg(valist, uint64_t), false);           break;
                        case '\0': print_hex(va_arg(valist, uint32_t), false);          break;
                    }
                    break;
                case 'X': // Uppercase hex
                    switch (info.length_specifier) {
                        case 'h': print_hex((uint16_t)va_arg(valist, uint32_t), true);  break;
                        case 'l': print_hex(va_arg(valist, uint64_t), true);            break;
                        case 'q': print_hex(va_arg(valist, uint64_t), true);            break;
                        case '\0': print_hex(va_arg(valist, uint32_t), true);           break;
                    }
                    break;
                case 'c': // Char
                    print_char((char)va_arg(valist, int));
                    break;
                case 'p': // Pointer address
                    print_pointer(va_arg(valist, uint64_t));
                    break;
                case 's': // String
                    print_str(va_arg(valist, char *));
                    break;
                default: // Invalid specifier
                    return -1;
            }
            end += info.length;
            start = end--;
        }
    } while (fmt[end++]);

    va_end(valist);

    return 1;
}