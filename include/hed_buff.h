#ifndef HE_BUFF_H
#define HE_BUFF_H

#include <stdlib.h>

#define HEBUFF_DEFAULT_CAPACITY 1024

// HEBuff stores the content to be desplayed on the screen on redrawing, one
// single buffer is used in order to prevent flickering
typedef struct {

    char *content; // Buffer content
    unsigned int len; // Current length of filled data
    unsigned int capacity; // The max allowed data for the current allocation

} HEBuff;

// Creates and initializes to 0's a new HEBuff
HEBuff* buff_create();

// Deallocates the memory including the object itself
void buff_remove(HEBuff* buff);

// Deletes the last character
void buff_delete_last(HEBuff* buff);

// Clear the buffer
void buff_clear(HEBuff* buff);

// Clear the buffer only setting len
void buff_clear_dirty(HEBuff* buff);

// Add `to_append` to the buffer
void buff_append(HEBuff* buff, const char* to_append, size_t len);

// Add formated string `fmt` to the buffer using va_args
void buff_vappendf(HEBuff* buff, const char* fmt, ...);

#endif
