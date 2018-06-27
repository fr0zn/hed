#ifndef HE_TERM_H
#define HE_TERM_H

#include "editor.h"
#include "buff.h"

// Gets the current terminal size and stores the value to the HEState struct
void term_get_size(HEState* state);

// Enables terminal raw mode
void term_enable_raw(HEState* state);

// Disable terminal raw mode
void term_disable_raw(HEState* state);

// Clear screen
void term_clear();

// Draws the buffer
void term_draw(HEBuff* buff);

#endif
