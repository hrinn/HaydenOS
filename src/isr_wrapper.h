#ifndef ISR_WRAPPER_H
#define ISR_WRAPPER_H

// Declarations of isr wrappers defined in assembly
extern void isr_wrapper_0(void);
extern void isr_wrapper_1(void);
extern void isr_wrapper_2(void);
extern void isr_wrapper_3(void);
extern void isr_wrapper_4(void);
extern void isr_wrapper_5(void);
extern void isr_wrapper_6(void);
extern void isr_wrapper_7(void);
extern void isr_wrapper_8(void);
extern void isr_wrapper_9(void);
extern void isr_wrapper_10(void);
extern void isr_wrapper_11(void);
extern void isr_wrapper_12(void);
extern void isr_wrapper_13(void);
extern void isr_wrapper_14(void);
extern void isr_wrapper_15(void);
extern void isr_wrapper_16(void);
extern void isr_wrapper_17(void);
extern void isr_wrapper_18(void);
extern void isr_wrapper_19(void);
extern void isr_wrapper_20(void);
extern void isr_wrapper_21(void);
extern void isr_wrapper_22(void);
extern void isr_wrapper_23(void);
extern void isr_wrapper_24(void);
extern void isr_wrapper_25(void);
extern void isr_wrapper_26(void);
extern void isr_wrapper_27(void);
extern void isr_wrapper_28(void);
extern void isr_wrapper_29(void);
extern void isr_wrapper_30(void);
extern void isr_wrapper_31(void);

// Isr wrapper table
isr_wrapper_t isr_wrapper_table[32] = {
    isr_wrapper_0, isr_wrapper_1, isr_wrapper_2, isr_wrapper_3,
    isr_wrapper_4, isr_wrapper_5, isr_wrapper_6, isr_wrapper_7,
    isr_wrapper_8, isr_wrapper_9, isr_wrapper_10, isr_wrapper_11,
    isr_wrapper_12, isr_wrapper_13, isr_wrapper_14, isr_wrapper_15,
    isr_wrapper_16, isr_wrapper_17, isr_wrapper_18, isr_wrapper_19,
    isr_wrapper_20, isr_wrapper_21, isr_wrapper_22, isr_wrapper_23,
    isr_wrapper_24, isr_wrapper_25, isr_wrapper_26, isr_wrapper_27,
    isr_wrapper_28, isr_wrapper_29, isr_wrapper_30, isr_wrapper_31
};

#endif