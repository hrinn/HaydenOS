#include "stdio.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "vga.h"
#include "string.h"

#define PRINTF_BUFF 256

// Converts a number to a string, places result in buffer
// Returns number of written digits including -
int itoa(int32_t num, char *buffer, int base, bool caps) {
    int i = 0, n = 0, rem;
    bool negative = false;

    if (buffer == NULL) {
        return 0;
    }

    // Handle 0
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return 1;
    }

    // Only treat base 10 numbers as signed
    if (base == 10 && num < 0) {
        negative = true;
        num *= -1;
    }

    // Convert number to string
    while (num > 0) {
        rem = num % base;
        buffer[i++] = (rem < 10) ? rem + '0' : 
            ((caps) ? (rem - 10) + 'A' : (rem - 10) + 'a');
        num /= base;
        n++;
    }

    // Append '-'
    if (negative) {
        buffer[i++] = '-';
        n++;
    }

    // Null terminate
    buffer[i] = '\0';

    // Reverse string
    strrev(buffer);

    return n;
}

// Converts a number to a string, places result in buffer
// Returns number of written digits including -
int uitoa(uint32_t num, char *buffer) {
    int i = 0, n = 0;

    if (buffer == NULL) {
        return 0;
    }

    // Handle 0
    if (num == 0) {
        buffer[i++] = '0';
        buffer[i] = '\0';
        return 1;
    }

    // Convert number to string
    while (num > 0) {
        buffer[i++] = (num % 10) + '0';
        num /= 10;
        n++;
    }

    // Null terminate
    buffer[i] = '\0';

    // Reverse string
    strrev(buffer);

    return n;
}

int printk(const char *fmt, ...) {
    va_list valist;
    int i = 0, len = strlen(fmt);
    char buff[PRINTF_BUFF];
    char *strarg, *bp = buff;

    if (fmt == NULL) return -1;

    va_start(valist, fmt);

    while (fmt[i] != '\0') {
        if (fmt[i] == '%' && i < len - 1) {
            // Check specifier
            switch (fmt[i+1]) { 
                case '%': // Percent
                    *bp++ = '%';
                    i++;
                    break;
                case 'd': // Signed integer
                    bp += itoa(va_arg(valist, int32_t), bp, 10, false);
                    i++;
                    break;
                case 'u': // Unsigned integer
                    bp += uitoa(va_arg(valist, uint32_t), bp);
                    i++;
                    break;
                case 'x': // hex
                    bp += itoa(va_arg(valist, uint32_t), bp, 16, false);
                    i++;
                    break;
                case 'X': // HEX
                    bp += itoa(va_arg(valist, uint32_t), bp, 16, true);
                    i++;
                    break;
                case 'c': // char
                    *bp++ = va_arg(valist, int);
                    i++;
                    break;
                case 'p': // Pointer address
                    strcpy(bp, "0x");
                    bp += 2;
                    bp += itoa((int32_t)va_arg(valist, void *), bp, 16, false);
                    i++;
                    break;
                case 'h':
                case 'l':
                case 'q':
                case 's':
                    strarg = va_arg(valist, char *);
                    strcpy(bp, strarg);
                    bp += strlen(strarg);
                    i++;
                    break;
                default: // Invalid specifier
                    return -1;
            }
        } else {
            *bp++ = fmt[i];
        }
        i++;
    }
    
    va_end(valist);
    *bp = '\0';

    VGA_display_str(buff);
}