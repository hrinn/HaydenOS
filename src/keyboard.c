#include "keyboard.h"
#include <stdint-gcc.h>
#include <stdbool.h>
#include <stddef.h>
#include "printk.h"
#include "ioport.h"
#include "debug.h"
#include "irq.h"
#include "proc.h"
#include "proc_queue.h"
#include "circ_buff.h"

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

// Other
#define N_TRIES 3
#define KEYBOARD_INT_LINE 1
#define KEYBOARD_IRQ 33

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
static char ASCII[] = "\0ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()~_+|\b \t\n{}:\"<>?";

// Keyboard state variables
typedef struct keyb_state {
    proc_queue_t blocked;
    buff_state_t circ_buff;
    uint8_t mod_state;
    uint64_t key_state;
} keyb_state_t;

static keyb_state_t keyb;

// Macros for accessing bits of state ints
#define SET_BIT(I, k) (I |= (1 << k))
#define CLEAR_BIT(I, k) (I &= ~(1 << k))
#define TEST_BIT(I, k) (I & (1 << k))

void keyboard_handler(uint8_t, uint32_t, void *);

uint8_t read_data() {
    while ((inb(PS2_STATUS) & 0x1) == 0); // Waiting for full output buffer
    return inb(PS2_DATA);
}

void write_data(uint8_t data) {
    while (inb(PS2_STATUS) & 0x2); // Waiting for empty input buffer
    outb(PS2_DATA, data);
}

static inline void write_command(uint8_t command) {
    outb(PS2_COMMAND, command);
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
    uint8_t resp = 0, tries = 0;
    do {
        write_data(byte);
        tries++;
    } while (tries < N_TRIES && (resp = read_data()) == KEYB_RESEND);

    return resp;
}

// Initializes the keyboard
// Returns 1 on success, negative on failure
int init_ps2_keyboard() {
    uint8_t resp;

    DEBUG_PRINT("Initializing keyboard\n");
    
    // Reset keyboard
    DEBUG_PRINT("Resetting keyboard and starting self test\n");
    if (write_keyboard(CMD_RST_KEYBOARD) != KEYB_ACK) {
        DEBUG_PRINT("Failed reset and self test\n");
        return -3;
    }

    // Set scan code set
    DEBUG_PRINT("Setting keyboard to scan code set 2\n");
    if ((resp = write_keyboard(CMD_SET_SCANCODE)) == KEYB_CONT) {
        if ((resp = write_keyboard(SUBCMD_SCANCODE_SET_2)) != KEYB_ACK) {
            DEBUG_PRINT("Failed to set scan code to set 2, received 0x%x\n", resp);
            return -5;
        }
    } else {
        DEBUG_PRINT("Failed to set scan code set, received 0x%x\n", resp);
        return -4;
    }

    // Enable keyboard scanning
    DEBUG_PRINT("Enabling keyboard scanning\n");
    if (write_keyboard(CMD_ENABLE_SCAN) != KEYB_ACK)  {
        DEBUG_PRINT("Failed to enable scanning\n");
        return -6;
    }

    return 1;
}

int KBD_init() {
    int res;
    if ((res = init_ps2_controller()) != 1) {
        return res;
    }
    if ((res = init_ps2_keyboard()) != 1) {
        return res;
    }
    // Initialize blocking queue
    PROC_init_queue(&keyb.blocked);

    // Initialize circular buffer
    init_buff(&keyb.circ_buff);

    // Initialize interrupts
    IRQ_set_handler(KEYBOARD_IRQ, keyboard_handler, NULL);
    IRQ_clear_mask(KEYBOARD_INT_LINE);

    return 1;
}

// Blocking function
char KBD_read() {
    char chr;
    wait_event_interruptable(&keyb.blocked, is_buffer_empty(&keyb.circ_buff));

    // Data in circular buffer
    consumer_read(&keyb.circ_buff, &chr);
    return chr;
}

// static bool release = false;

void keyboard_handler(
    __attribute__((unused)) uint8_t irq, 
    __attribute__((unused)) uint32_t error_code, 
    __attribute__((unused)) void *arg) 
{
    enum key key = nokey;
    enum modifier mod = nomod;
    static bool release = false;
    char chr;

    switch (inb(PS2_DATA)) {
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
        default: break; // unsupported key
    }

    if (key != nokey) {
        if (release) {
            release = false;
            CLEAR_BIT(keyb.key_state, key);
        } else {
            SET_BIT(keyb.key_state, key);
            // Character ready, add to circular buffer
            chr = TEST_BIT(keyb.mod_state, shift) ? ASCII[key] : ascii[key];
            producer_write(chr, &keyb.circ_buff);
            // Unblock process waiting to ready
            PROC_unblock_head(&keyb.blocked);
        }
    }     

    if (mod != nomod) {
        if (release) {
            release = false;
            CLEAR_BIT(keyb.mod_state, mod);
        } else {
            SET_BIT(keyb.mod_state, mod);
        }
    }

    IRQ_end_of_interrupt(KEYBOARD_INT_LINE);
}