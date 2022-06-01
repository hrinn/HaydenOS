#include "../kernel/sys_call_ints.h"

void main(void *arg) {
    while (1) {
        putc(getc());
    }
}