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

// Responses
#define PS2_TEST_RESPONSE 0x55

static inline uint8_t read_data() {
    while ((inb(PS2_STATUS) & 0x1) == 0); // Waiting for full output buffer
    return inb(PS2_DATA);
}

static inline void write_data(uint8_t data) {
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

    DEBUG_PRINT(("Initializing PS/2 Controller\n"));

    // Disable ports
    DEBUG_PRINT(("Disabling ports\n"));
    write_command(CMD_DISABLE_P1);
    write_command(CMD_DISABLE_P2);

    // Flush output buffer
    inb(PS2_DATA);

    // Configure controller
    DEBUG_PRINT(("Configuring controller\n"));
    write_command(CMD_READ_CONFIG_BYTE);
    data = inb(PS2_DATA);
    data |= (CFG_CLK1 | CFG_INT1); // Enable first clock and interrupt
    data &= ~(CFG_CLK2 | CFG_INT2 | CFG_TRANS); // Disable second clock, interrupt, and translation
    write_command(CMD_WRITE_CONFIG_BYTE);
    write_data(data);

    // Self test
    DEBUG_PRINT(("Testing controller\n"));
    write_command(CMD_TEST_PS2);
    if (read_data() != PS2_TEST_RESPONSE) return -1;

    // Test port one
    DEBUG_PRINT(("Testing port 1\n"));
    write_command(CMD_TEST_P1);
    if (read_data() != 0) return -2;

    // Enable port one
    DEBUG_PRINT(("Enabling port 1\n"));
    write_command(CMD_ENABLE_P1);

    return 1;
}

// Initializes the keyboard
// Returns 1 on success, negative on failure
int init_keyboard() {
    DEBUG_PRINT(("Initializing keyboard\n"));
    // Reset keyboard
    DEBUG_PRINT(("Resetting keyboard\n"));
    write_data(CMD_RST_KEYBOARD);

    return 1;
}

