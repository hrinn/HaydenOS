#ifndef DEBUG_H
#define DEBUG_H

#include "printk.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) printk (__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

#endif