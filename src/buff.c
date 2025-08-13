#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <hed_buff.h>

// Allocate and initialize a HEDBuff with default size data
HEDBuff* buff_create(void){

    HEDBuff *buff = malloc(sizeof(HEDBuff));
    if(buff == NULL) {
        perror("failed to allocate memory for buffer: ");
        exit(1);
    }

    buff->content  = malloc(HEDBUFF_DEFAULT_CAPACITY);
    if(buff->content == NULL) {
        perror("failed to allocate memory for buffer: ");
        exit(1);
    }
    buff->len      = 0;
    buff->capacity = HEDBUFF_DEFAULT_CAPACITY;
    // Make sure empty string of len 0
    buff->content[0] = 0;
    return buff;
}

void buff_remove(HEDBuff* buff){
    if(buff != NULL){
        if(buff->content != NULL){
            free(buff->content);
        }
        free(buff);
    }
}

// Sets the length to 0 in order to start appending data from the beggining of
// the buffer again. The data is not cleared, only the first byte set to 0
void buff_clear_dirty(HEDBuff* buff) {
    buff->content[0] = 0;
    buff->len = 0;
}

// Sets the length to 0 in order to start appending data from the beggining of
// the buffer again. The capacity will stay the same and the buffer content
// pointer will be the same also. The old data is set to 0
void buff_clear(HEDBuff* buff){
    //memset(buff->content, 0, buff->len + 1);
    //buff->len = 0;
    buff_clear_dirty(buff);
}

void buff_delete_last(HEDBuff* buff){
    if(buff->len > 0 ){
        buff->content[buff->len-1] = 0;
        buff->len--;
    }
}

// Appends `to_append` to the HEDBuff content and reallocates in case is needed
void buff_append(HEDBuff* buff, const char* to_append, size_t len) {

    // If the HEDBuff len exceeds the capacity, reallocate and double the
    // capacity
    if( ( buff->len + len + 1 ) >= buff->capacity ){
        buff->capacity += len; // Increment the capacity to fit the data
        buff->capacity *= 2; // Double the size of the capacity
        buff->content = realloc(buff->content, buff->capacity);
        if ( buff->content == NULL ){
            perror("Unable to realloc HEDBuff content");
            exit(1);
        }
    }

    memcpy(buff->content + buff->len, to_append, len);
    buff->len += len;
    // Add null byte at the end of memcpy
    buff->content[buff->len] = 0;
}

int buff_vappendf(HEDBuff* buff, const char* fmt, ...) {

    char buffer[HEDBUFF_DEFAULT_CAPACITY];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buffer, HEDBUFF_DEFAULT_CAPACITY, fmt, ap);
    va_end(ap);

    buff_append(buff, buffer, len);
    return len;
}
