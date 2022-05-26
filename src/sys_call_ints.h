#ifndef USER_SYS_CALL_H
#define USER_SYS_CALL_H

#define YIELD_SYS_CALL 0
#define KEXIT_SYS_CALL 1
#define GETC_SYS_CALL 2
#define PUTC_SYS_CALL 3

static inline void yield(void) {
    asm volatile ( "movq $0, %rdi");
    asm volatile ( "movq $0, %rsi");
    asm volatile ( "INT $206" );
}

static inline void kexit(void) {
    asm volatile ( "movq $1, %rdi");
    asm volatile ( "movq $0, %rsi");
    asm volatile ( "INT $206" );
}

static inline char getc(void) {
    char ret;
    asm volatile ( "movq $2, %rdi");
    asm volatile ( "movq $0, %rsi");
    asm volatile ( "INT $206" );
    asm ( "movb %%dil, %0" : "=r"(ret));
    return ret;
}

static inline void putc(char c) {
    asm volatile ( "movq $3, %rdi");
    asm volatile ( "movq %0, %%rsi" : : "r"((uint64_t)c));
    asm volatile ( "INT $206" );
}

#endif