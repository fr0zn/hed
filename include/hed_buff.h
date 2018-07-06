#ifndef HED_BUFF_H
#define HED_BUFF_H

#include <stdlib.h>
#include <hed_types.h>

#define HEDBUFF_DEFAULT_CAPACITY 1024

/**
 * { item_description }
 */
typedef struct {
    char *content;
    // Current length of filled data
    unsigned int len;
    // The max allowed data for the current allocation
    unsigned int capacity;
} HEDBuff;

// Creates and initializes to 0's a new HEDBuff
HEDBuff* buff_create();

// Deallocates the memory including the object itself
void buff_remove(HEDBuff* buff);

// Deletes the last character
void buff_delete_last(HEDBuff* buff);

// Removes trailing whitespaces
void buff_trim(HEDBuff* buff);

// Clear the buffer
void buff_clear(HEDBuff* buff);

// Clear the buffer only setting len
void buff_clear_dirty(HEDBuff* buff);

// Add `to_append` to the buffer
void buff_append(HEDBuff* buff, const char* to_append, size_t len);

// Add formated string `fmt` to the buffer using va_args
int buff_vappendf(HEDBuff* buff, const char* fmt, ...);

// Appends another buffer
void buff_append_buff(HEDBuff* buff, HEDBuff* buff_to_append);

#endif
