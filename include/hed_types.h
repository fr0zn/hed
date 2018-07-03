#ifndef HED_TYPES_H
#define HED_TYPES_H

#include <stdint.h>

/**
 * Enum of the ANSI Character Escape sequences
 * The values for an specific keyboard can be optained with the
 * python script `helpers/key_code.py`
 */
typedef enum {
    KEY_NULL      = 0,
    KEY_CTRL_C    = 0x03,
    KEY_CTRL_D    = 0x04,
    KEY_CTRL_H    = 0x08,
    KEY_TAB       = 0x09,
    KEY_CTRL_Q    = 0x11,
    KEY_CTRL_R    = 0x12,
    KEY_CTRL_S    = 0x13,
    KEY_CTRL_U    = 0x15,
    KEY_ESC       = 0x1b,
    KEY_ENTER     = 0x0d,
    KEY_BACKSPACE = 0x7f,

    /**
     * The following values are manually defined in order to represent
     * escaped key sequence of the corresponding name key.
     */

    KEY_UP      = 0x400,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_DEL,
    KEY_HOME,
    KEY_END,
    KEY_PAGEUP,
    KEY_PAGEDOWN,
}KEY_CODE;

/**
 * Defines a byte type of size 1 byte in memory with nibble access
 * Example:
 *        byte_t b;
 *        b.nibble.top = 4;
 *        b.nibble.bottom = 6;
 *
 *        printf("0x%x", b.byte); // Outputs 0x46
 */
typedef union {
    struct {
        unsigned char bottom : 4;
        unsigned char top : 4;
    }nibble;

    unsigned char value;
} byte_t;

/**
 * Defines a byte type used by HeD editor
 * <details>
 */
typedef struct {
    byte_t o; // Original
    byte_t c; // Current
    int g; // Grammar index
} HEDByte;

enum action_type {
    ACTION_BASE, // Used as the base of actions
    ACTION_REPLACE,
    ACTION_INSERT,
    ACTION_DELETE,
    ACTION_APPEND
};

static const char* action_names[] = {
        "base",
        "replace",
        "insert",
        "delete",
        "append"
};

typedef struct action_t{
    struct action_t *prev;
    struct action_t *next;

    enum action_type type;
    unsigned int offset;

    HEDByte b; // Changed/modified byte (original)

}HEAction;

typedef struct {
    HEAction *first;
    HEAction *last;
    HEAction *current; // So we can redo

}HEActionList;

#endif
