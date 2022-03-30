#include "keyboard.h"
#include <stdint.h>
#include "printk.h"
#include "ioport.h"
#include "debug.h"

// I/O Port Addresses
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64
#define PS2_DATA 0x60

// PS/2 Commands
#define CMD_DISABLE_P1 0xAD
#define CMD_DISABLE_P2 0xA7
#define CMD_READ_CONFIG_BYTE 0x20
#define CMD_WRITE_CONFIG_BYTE 0x60
#define CMD_TEST_PS2 0xAA
#define CMD_TEST_P1 0xAB
#define CMD_ENABLE_P1 0xAE

// PS/2 Configuration Bits
#define CFG_INT1 0x1
#define CFG_INT2 0x2
#define CFG_CLK1 0x10
#define CFG_CLK2 0x20
#define CFG_TRANS 0x40

// Keyboard Commands
#define CMD_RST_KEYBOARD 0xFF
#define CMD_ENABLE_SCAN 0xF4
#define CMD_SET_SCANCODE 0xF0
#define SUBCMD_SCANCODE_SET_2 2

// Responses
#define PS2_TEST_PASS 0x55
#define KEYB_RESEND 0xFE
#define KEYB_ACK 0xFA
#define KEYB_CONT 0xAA

#define N_TRIES 3

uint8_t read_data() {
    while ((inb(PS2_STATUS) & 0x1) == 0); // Waiting for full output buffer
    return inb(PS2_DATA);
}

void write_data(uint8_t data) {
    while (inb(PS2_STATUS) & 0x2); // Waiting for empty input buffer
    outb(data, PS2_DATA);
}

static inline void write_command(uint8_t command) {
    outb(command, PS2_COMMAND);
}

// Initializes the PS/2 Controller
// Returns 1 on success, negative on failure
int init_ps2_controller() {
    uint8_t data;

    DEBUG_PRINT("Initializing PS/2 Controller\n");

    // Disable ports
    DEBUG_PRINT("Disabling ports\n");
    write_command(CMD_DISABLE_P1);
    write_command(CMD_DISABLE_P2);

    // Flush output buffer
    inb(PS2_DATA);

    // Configure controller
    DEBUG_PRINT("Configuring controller\n");
    write_command(CMD_READ_CONFIG_BYTE);
    data = inb(PS2_DATA);
    data |= (CFG_CLK1 | CFG_INT1); // Enable first clock and interrupt
    data &= ~(CFG_CLK2 | CFG_INT2 | CFG_TRANS); // Disable second clock, interrupt, and translation
    write_command(CMD_WRITE_CONFIG_BYTE);
    write_data(data);

    // Self test
    DEBUG_PRINT("Testing controller\n");
    write_command(CMD_TEST_PS2);
    if (read_data() != PS2_TEST_PASS) return -1;

    // Test port one
    DEBUG_PRINT("Testing port 1\n");
    write_command(CMD_TEST_P1);
    if (read_data() != 0) return -2;

    // Enable port one
    DEBUG_PRINT("Enabling port 1\n");
    write_command(CMD_ENABLE_P1);

    return 1;
}

// Writes to the keyboard, waiting for ACK and resending if requested
// Returns response
uint8_t write_keyboard(uint8_t byte) {
    uint8_t resp, tries = 0;
    do {
        write_data(byte);
        tries++;
    } while (tries < N_TRIES && (resp = read_data()) == KEYB_RESEND);

    return resp;
}

// Initializes the keyboard
// Returns 1 on success, negative on failure
int init_keyboard() {
    uint8_t resp;

    DEBUG_PRINT("Initializing keyboard\n");
    
    // Reset keyboard
    DEBUG_PRINT("Resetting keyboard and starting self test\n");
    if (write_keyboard(CMD_RST_KEYBOARD) != KEYB_ACK) {
        DEBUG_PRINT("Failed reset and self test\n");
        return -1;
    }

    // Set scan code set
    DEBUG_PRINT("Setting keyboard to scan code set 2\n");
    if ((resp = write_keyboard(CMD_SET_SCANCODE)) == KEYB_CONT) {
        if ((resp = write_keyboard(SUBCMD_SCANCODE_SET_2)) != KEYB_ACK) {
            DEBUG_PRINT("Failed to set scan code to set 2, received 0x%x\n", resp);
            return -3;
        }
    } else {
        DEBUG_PRINT("Failed to set scan code set, received 0x%x\n", resp);
        return -2;
    }

    // Enable keyboard scanning
    DEBUG_PRINT("Enabling keyboard scanning\n");
    if (write_keyboard(CMD_ENABLE_SCAN) != KEYB_ACK)  {
        DEBUG_PRINT("Failed to enable scanning\n");
        return -4;
    }

    return 1;
}

// Read data from the keyboard and translate it into key presses
char poll_keyboard() {
    return read_data();
}