#ifndef HE_TERM_H
#define HE_TERM_H

#include "editor.h"
#include "buff.h"
#include <unistd.h>

// Gets the current terminal size and stores the value to the HEState struct
void term_get_size(HEState* state);

// Enables terminal raw mode
void term_enable_raw(HEState* state);

// Disable terminal raw mode
void term_disable_raw(HEState* state);

// Clear screen
void term_clear();

// Prints a char array
void term_print(char *data, ssize_t len);

// Draws the buffer
void term_draw(HEBuff* buff);

#endif
