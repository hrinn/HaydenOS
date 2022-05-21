#include "serial.h"
#include "ioport.h"
#include "irq.h"
#include "printk.h"
#include "registers.h"
#include <stddef.h>
#include <stdint-gcc.h>
#include "circ_buff.h"
#include "vga.h"
#include "proc.h"

#define COM1 0x3F8
#define HW_BUFF_SIZE 14
#define SERIAL_IRQ 36
#define IIR_ID_MASK 0x0E
#define IIR_TX_EMPTY 0x02
#define IIR_LINE_STATUS 0x06

// Function declarations
void serial_isr(uint8_t irq, uint32_t error_code, void *arg);

typedef enum {IDLE, BUSY} hw_status_t;

static buff_state_t state;
static hw_status_t status;
static proc_queue_t blocked;

void SER_kspace_offset(uint64_t offset) {
    state.write_head += offset;
    state.read_head += offset;
}

// Writes a byte to the UART
void TX_byte(char data) {
    outb(COM1, data);
}

// Initializes state and UART
// Returns 1 on success, -1 on failure
int SER_init(void) {
    init_buff(&state);

    // Disable all UART interrupts
    outb(COM1 + 1, 0x00);   

    // Initialize interrupt handler
    IRQ_set_handler(SERIAL_IRQ, serial_isr, NULL);
    IRQ_clear_mask(SERIAL_IRQ);

    // Initialize queue for later blocking
    PROC_init_queue(&blocked);

    // Initialize UART
    outb(COM1 + 3, 0x80);   // Enable DLAB
    outb(COM1 + 0, 0x01);   // 112500 baud
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
void init_hw_write() {
    int i = 0;
    char c;

    if (status == IDLE) {
        // Fill the internal hardware buffer
        while (i < HW_BUFF_SIZE && consumer_read(&state, &c)) {
            TX_byte(c);
            i++;
        } 
        if (i > 0) status = BUSY;
    }
    
    // We are either done or waiting for a TX empty interrupt
}

// Writes a buffer of characters to the circular buffer
// Returns number of characters written
int SER_write(const char *buff, int len) {
    int i = 0;
    uint16_t int_en = check_int();
    if (int_en) CLI;

    while (buff[i] && i < len) {
        if (producer_write(buff[i], &state)) {
            i++;
        } else {
            // Buffer was full, drop the data
            break;
        }
    }
    
    init_hw_write();
    if (int_en) STI;
    return i;
}

// Writes a buffer of characters to the circular serial buffer
// Blocks when the buffer fills
// Returns the number of characters written
int SER_writeb(const char *buff, int len) {
    int i = 0;
    CLI;

    while (buff[i] && i < len) {
        if (producer_write(buff[i], &state)) {
            i++;
        } else {
            // Buffer was full
            // Initialize HW write and block
            init_hw_write();
            wait_event_interruptable(&blocked, is_buffer_full(&state));
        }
    }

    init_hw_write();
    STI;
    return i;
}

// Services two interrupts
// 1: TX interrupt - occurs when TX buffer empties
// 2: LINE interrupt - line status register needs to be read
void serial_isr(uint8_t irq, uint32_t error_code, void *arg) {
    // Check interrupt type
    switch (inb(COM1 + 2) & IIR_ID_MASK) {
        case IIR_TX_EMPTY:      // Transmit
            status = IDLE;
            init_hw_write();
            break;
        case IIR_LINE_STATUS:   // Read line status register
            inb(COM1 + 5);
            break;
        default:
            VGA_display_str("Bad serial isr!\n", 17);
            break;
    }

    PROC_unblock_head(&blocked);
    IRQ_end_of_interrupt(SERIAL_IRQ);
}