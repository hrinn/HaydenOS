#ifndef SYS_CALL_H
#define SYS_CALL_H

#include <stdint-gcc.h>

typedef uint64_t (*sys_call_f)(uint64_t);
#define SYS_CALL_IST 4
#define SYS_CALL_IRQ 206

void init_sys_calls();
void set_sys_call(uint8_t num, sys_call_f sys_call);

#endif