#include "sys_call_ints.h"

void main(int argc, void **argv) {
    while (1) {
        putc(getc());
    }
}