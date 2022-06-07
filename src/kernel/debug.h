#ifndef DEBUG_H
#define DEBUG_H

#ifdef GDB
#define GDB_PAUSE int gdbp = 0; while (!gdbp)
#else
#define GDB_PAUSE
#endif

#endif