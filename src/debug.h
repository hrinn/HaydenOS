#ifndef DEBUG_H
#define DEBUG_H

#include "printk.h"

#define DEBUG

#ifdef DEBUG
#define DEBUG_PRINT(x) printk x
#else
#define DEBUG_PRINT(x) do {} while (0);
#endif

#endif