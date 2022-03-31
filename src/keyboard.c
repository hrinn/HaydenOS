#include "keyboard.h"
#include <stdint.h>
#include <stdbool.h>
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

// Operation scan codes
#define SC_BREAK 0xF0

// Alpha numeric scan codes
#define SC_A 0x1C
#define SC_B 0x32
#define SC_C 0x21
#define SC_D 0x23
#define SC_E 0x24
#define SC_F 0x2B
#define SC_G 0x34
#define SC_H 0x33
#define SC_I 0x43
#define SC_J 0x3B
#define SC_K 0x42
#define SC_L 0x4B
#define SC_M 0x3A
#define SC_N 0x31
#define SC_O 0x44
#define SC_P 0x4D
#define SC_Q 0x15
#define SC_R 0x2D
#define SC_S 0x1B
#define SC_T 0x2C
#define SC_U 0x3C
#define SC_V 0x2A
#define SC_W 0x1D
#define SC_X 0x22
#define SC_Y 0x35
#define SC_Z 0x1A
#define SC_0 0x45
#define SC_1 0x16
#define SC_2 0x1E
#define SC_3 0x26
#define SC_4 0x25
#define SC_5 0x2E
#define SC_6 0x36
#define SC_7 0x3D
#define SC_8 0x3E
#define SC_9 0x46

// Special scan codes
#define SC_BACKTICK 0x0E
#define SC_DASH 0x4E
#define SC_EQUALS 0x55
#define SC_BACKSLASH 0x5D
#define SC_BKSP 0x66
#define SC_SPACE 0x29
#define SC_TAB 0x0D
#define SC_ENTER 0x5A
// #define SC_ESC 0x76
#define SC_LBRACKET 0x54
#define SC_RBRACKET 0x5B
#define SC_SEMICOLON 0x4C
#define SC_SQUOTE 0x52
#define SC_COMMA 0x41
#define SC_PERIOD 0x49
#define SC_FWDSLASH 0x4A

// Modifier scan codes
#define SC_LSHFT 0x12
#define SC_LCTRL 0x14
#define SC_LALT 0x11
#define SC_RSHFT 0x59

// Key and mod enums for indexing
enum modifier {nomod, shift, control, alt};
enum key {nokey, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, \
    r, s, t, u, v, w, x, y, z, k1, k2, k3, k4, k5, k6, k7, k8, k9, k0, \
    backtick, dash, equals, backslash, bksp, space, tab, enter, lbracket, \
    rbracket, semicolon, squote, comma, period, fwdslash};

// Strings for actual chars
static char ascii[] = "\0abcdefghijklmnopqrstuvwxyz1234567890`-=\\\b \t\n[];\',./";
static char uascii[] = "\0ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()~_+|\b \t\n{}:\"<>?";

// Arrays for tracking key states
static uint8_t mod_state;
static uint64_t key_state;

// Macros for accessing bits of state ints
#define SET_BIT(I, k) (I |= (1 << k))
#define CLEAR_BIT(I, k) (I &= ~(1 << k))
#define TEST_BIT(I, k) (I & (1 << k))

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
    uint8_t config;

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
    config = inb(PS2_DATA);
    config |= (CFG_CLK1 | CFG_INT1); // Enable first clock and interrupt
    config &= ~(CFG_CLK2 | CFG_INT2 | CFG_TRANS); // Disable second clock, interrupt, and translation
    write_command(CMD_WRITE_CONFIG_BYTE);
    write_data(config);

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
    bool release = false;
    enum key key;
    enum modifier mod;

    while (1) {
        key = nokey;
        mod = nomod;

        switch(read_data()) {
            case SC_BREAK:
                release = true; break;
            case SC_A:
                key = a; break;
            case SC_B:
                key = b; break;
            case SC_C:
                key = c; break;
            case SC_D:
                key = d; break;
            case SC_E:
                key = e; break;
            case SC_F:
                key = f; break;
            case SC_G:
                key = g; break;
            case SC_H:
                key = h; break;
            case SC_I:
                key = i; break;
            case SC_J:
                key = j; break;
            case SC_K:
                key = k; break;
            case SC_L:
                key = l; break;
            case SC_M:
                key = m; break;
            case SC_N:
                key = n; break;
            case SC_O:
                key = o; break;
            case SC_P:
                key = p; break;
            case SC_Q:
                key = q; break;
            case SC_R:
                key = r; break;
            case SC_S:
                key = s; break;
            case SC_T:
                key = t; break;
            case SC_U:
                key = u; break;
            case SC_V:
                key = v; break;
            case SC_W:
                key = w; break;
            case SC_X:
                key = x; break;
            case SC_Y:
                key = y; break;
            case SC_Z:
                key = z; break;
            case SC_0:
                key = k0; break;
            case SC_1:
                key = k1; break;
            case SC_2:
                key = k2; break;
            case SC_3:
                key = k3; break;
            case SC_4:
                key = k4; break;
            case SC_5:
                key = k5; break;
            case SC_6:
                key = k6; break;
            case SC_7:
                key = k7; break;
            case SC_8:
                key = k8; break;
            case SC_9:
                key = k9; break;
            case SC_BACKTICK:
                key = backtick; break;
            case SC_DASH:
                key = dash; break;
            case SC_EQUALS:
                key = equals; break;
            case SC_BACKSLASH:
                key = backslash; break;
            case SC_BKSP:
                key = bksp; break;
            case SC_SPACE:
                key = space; break;
            case SC_TAB:
                key = tab; break;
            case SC_ENTER:
                key = enter; break;
            case SC_LBRACKET:
                key = lbracket; break;
            case SC_RBRACKET:
                key = rbracket; break;
            case SC_SEMICOLON:
                key = semicolon; break;
            case SC_SQUOTE:
                key = squote; break;
            case SC_COMMA:
                key = comma; break;
            case SC_PERIOD:
                key = period; break;
            case SC_FWDSLASH:
                key = fwdslash; break;
            case SC_LSHFT:
                mod = shift; break;
            case SC_LCTRL:
                mod = control; break;
            case SC_LALT:
                mod = alt; break;
            case SC_RSHFT:
                mod = shift; break;
            default: // Unsupported scancode
                break;
        }

        if (key != nokey) {
            if (release) {
                release = false;
                CLEAR_BIT(key_state, key);
            } else {
                SET_BIT(key_state, key);
                // Send key on press
                return TEST_BIT(mod_state, shift) ? uascii[key] : ascii[key];
            }
        }

        if (mod != nomod) {
            if (release) {
                release = false;
                CLEAR_BIT(mod_state, mod);
            } else {
                SET_BIT(mod_state, mod);
            }
        }
    }
}