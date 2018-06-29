#ifndef HE_UTILS_H
#define HE_UTILS_H

#include <stdint.h>

int utils_read_key();
unsigned char utils_atoh (unsigned char chr);

// Key enumeration, returned by utils_read_key().
enum key_codes {
    KEY_NULL      = 0,
    KEY_CTRL_D    = 0x04,
    KEY_CTRL_H    = 0x08,
    KEY_TAB       = 0x09,
    KEY_CTRL_Q    = 0x11, // DC1, to exit the program.
    KEY_CTRL_R    = 0x12, // DC2, to redo an action.
    KEY_CTRL_S    = 0x13, // DC3, to save the current buffer.
    KEY_CTRL_U    = 0x15,
    KEY_ESC       = 0x1b, // ESC, for things like keys up, down, left, right, delete, ...
    KEY_ENTER     = 0x0d,
    KEY_BACKSPACE = 0x7f,

    KEY_UP      = 1000, // [A
    KEY_DOWN,           // [B
    KEY_RIGHT,          // [C
    KEY_LEFT,           // [D
    KEY_DEL,            // . = 1b, [ = 5b, 3 = 33, ~ = 7e,
    KEY_HOME,           // [H
    KEY_END,            // [F
    KEY_PAGEUP,         // ??
    KEY_PAGEDOWN,       // ??
};

typedef struct {
    uint8_t o; // Original
    uint8_t c; // Current
} byte_t;

#endif
