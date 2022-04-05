#ifndef DEBUG_H
#define DEBUG_H

#include "printk.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printk (__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#ifdef GDB
#define GDB_PAUSE int gdbp = 0; while (!gdbp)
#else
#define GDB_PAUSE
#endif

#endif