#ifndef INIT_SYSTEM_CALLS_H
#define INIT_SYSTEM_CALLS_H

#include <stdint-gcc.h>

typedef uint64_t (*sys_call_f)(uint64_t);
#define SYS_CALL_IRQ 206

void init_sys_calls();
void set_sys_call(uint8_t num, sys_call_f sys_call);

#endif