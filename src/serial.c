#include "serial.h"
#include "ioport.h"
#include "irq.h"
#include "printk.h"
#include <stddef.h>
#include <stdint.h>

#define COM1 0x3F8
#define BUFF_SIZE 16
#define HW_BUFF_SIZE 14
#define SERIAL_IRQ 36
#define SERIAL_INT_LINE 4
#define IIR_ID_MASK 0x0E
#define IIR_TX_EMPTY 0x02
#define IIR_LINE_STATUS 0x06

// Function declarations
void serial_isr(uint8_t irq, uint32_t error_code, void *arg);

typedef enum {IDLE, BUSY} hw_status_t;

// State of circular buffer
typedef struct {
    char buff[BUFF_SIZE + 1]; // 1 byte is wasted to differentiate b/t empty and full
    char *head;
    char *tail;
    hw_status_t status;
} state_t;

static state_t uart_state;

// Initializes circular buffer state
void init_state(state_t *state) {
    state->head = &state->buff[0];
    state->tail = &state->buff[0];
}

// Writes a byte to the UART
static inline void TX_byte(uint8_t data) {
    outb(COM1, data);
}

// Consumes a char from the circular buffer
// Writes this char to the UART
// Returns 1 on success, 0 if nothing to read
int consumer_read(state_t *state) {
    if (state->head == state->tail) {
        return 0;
    }

    TX_byte(*state->head++);
    if (state->head >= &state->buff[BUFF_SIZE]) {
        state->head = &state->buff[0];
    }

    return 1;
}

// Writes a char to the circular buffer
// Returns 1 on success, 0 if buffer full
int producer_write(char c, state_t *state) {
    if (state->head == state->tail + 1 || 
        (state->head == &state->buff[0] && state->tail == &state->buff[BUFF_SIZE - 1])) {
        return 0;
    }

    *state->tail++ = c;
    if (state->tail >= &state->buff[BUFF_SIZE]) {
        state->tail = &state->buff[0];
    }

    return 1;
}

// Initializes state and UART
// Returns 1 on success, -1 on failure
int SER_init(void) {
    init_state(&uart_state);

    // Disable all UART interrupts
    outb(COM1 + 1, 0x00);   

    // Initialize interrupt handler
    IRQ_set_handler(SERIAL_IRQ, serial_isr, NULL);
    IRQ_clear_mask(SERIAL_INT_LINE);

    // Initialize UART
    outb(COM1 + 3, 0x80);   // Enable DLAB
    outb(COM1 + 0, 0x01);   // Set divisor to 1 (full throttle)
    outb(COM1 + 1, 0x00);   // ^
    outb(COM1 + 3, 0x03);   // 8 bits, no parity, 1 stop bit, disable DLAB
    outb(COM1 + 2, 0xC7);   // Enable and clear 14 byte FIFO
    outb(COM1 + 4, 0x0B);   // Set IRQs, RTS, and DTR
    outb(COM1 + 4, 0x1E);   // Set in loopback mode, test the serial chip
    outb(COM1 + 0, 0xAE);   // Test serial chip, echo send

    if (inb(COM1 + 0) != 0xAE) {
        return -1;
    }

    outb(COM1 + 4, 0x0F);   // Set in normal operation mode
    outb(COM1 + 1, 0x02);   // Set TX IRQ only
    return 1;
}

int is_tx_empty() {
    return inb(COM1 + 5) & 0x20;
}

// Reads from the buffer of characters
void init_hw_write(state_t *state) {
    int i = 0;

    if (state->status != IDLE) {
        while(!is_tx_empty());
        state->status = IDLE;
    }

    state->status = BUSY;
    // Write up to 14 bytes the internal hardware buffer
    while (i++ < HW_BUFF_SIZE && consumer_read(state));
    // We are either done or waiting for a TX empty interrupt
}

// Writes a buffer of characters to the circular buffer
// Returns number of characters written
int SER_write(const char *buff, int len) {
    int i = 0;
    uint16_t int_en = check_int();
    if (int_en) cli();

    while (buff[i] && i < len) {
        if (producer_write(buff[i], &uart_state)) {
            i++;
        } else {
            // Buffer was full
            init_hw_write(&uart_state);
        }
    }
    
    init_hw_write(&uart_state);
    if (int_en) sti();
    return len;
}

// Services two interrupts
// 1: TX interrupt - occurs when TX buffer empties
// 2: LINE interrupt - line status register needs to be read
void serial_isr(uint8_t irq, uint32_t error_code, void *arg) {
    // Check interrupt type
    switch (inb(COM1 + 2) & IIR_ID_MASK) {
        case IIR_TX_EMPTY:      // Transmit
            uart_state.status = IDLE;
            init_hw_write(&uart_state);
            break;
        case IIR_LINE_STATUS:   // Read line status register
            inb(COM1 + 5);
            break;
        default:
            // Bad news
            return;
    }
}