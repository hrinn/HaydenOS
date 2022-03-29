#include "keyboard.h"
#include <stdint.h>
#include "printk.h"

// PS/2 Commands
#define DISABLE_P1 0xAD
#define DISABLE_P2 0xA7
#define READ_CONFIG_BYTE 0x20
#define WRITE_CONFIG_BYTE 0x60
#define TEST_PS2 0xAA
#define TEST_P1 0xAB
#define ENABLE_P1 0xAE

// Keyboard Commands
#define RESET_KEYBOARD 0xFF

// MMIO
uint8_t * const command_reg = (uint8_t *)0x64;
const uint8_t * const status_reg = command_reg;
uint8_t * const data_port = (uint8_t *)0x60;

uint8_t read_data() {
    while (!(*status_reg & 0x1));  // Wait for full output buffer
    return *data_port;
}

void write_data(uint8_t data) {
    while (*status_reg & 0x2);     // Wait for empty input buffer
    *data_port = data;
}

void write_command(uint8_t command) {
    while (*status_reg & 0x2);      // Wait for empty input buffer
    *command_reg = command;
}

// Initializes the PS/2 Controller
// Returns 1 on success, negative on failure
int init_ps2_controller() {
    uint8_t data;

    // Disable ports
    printk("Disabling ports...");
    write_command(DISABLE_P1);
    write_command(DISABLE_P2);

    // Flush output buffer
    data = *data_port;

    // Configure controller
    printk("Configuring controller...");
    write_command(READ_CONFIG_BYTE);
    data = read_data();
    write_command(WRITE_CONFIG_BYTE);
    write_data(data & 0xBC); // Disables IRQs and translation

    // Self test
    printk("Running self test...");
    write_command(TEST_PS2);
    if (read_data() != 0x55) return -1;

    // Test port one
    printk("Testing port one...");
    write_command(TEST_P1);
    if (read_data() != 0) return -2;

    // Enable port one
    printk("Enabling port one...");
    write_command(ENABLE_P1);

    return 1;
}

