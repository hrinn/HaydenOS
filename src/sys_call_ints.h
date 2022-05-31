#ifndef SYS_CALL_INTS_H
#define SYS_CALL_INTS_H

#define YIELD_SYS_CALL 0
#define KEXIT_SYS_CALL 1
#define GETC_SYS_CALL 2
#define PUTC_SYS_CALL 3

extern void yield(void);
extern void kexit(void);
extern char getc(void);
extern void putc(char);

#endif