#include "sys_call_ints.h"

void main() {
    while (1) {
        putc(getc());
    }
}