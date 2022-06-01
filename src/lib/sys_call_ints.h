#ifndef SYS_CALL_INTS_H
#define SYS_CALL_INTS_H

#define YIELD_SYS_CALL 0
#define GETC_SYS_CALL 1
#define PUTC_SYS_CALL 2

extern void yield(void);
extern void kexit(void);
extern char getc(void);
extern void putc(char);

#endif