#include "buff.h"
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

// Allocate and initialize a HEBuff with default size data
HEBuff* buff_create(){

    HEBuff *buff = malloc(sizeof(HEBuff));

    if(buff){
        buff->content  = malloc(HEBUFF_DEFAULT_CAPACITY);
        buff->len      = 0;
        buff->capacity = HEBUFF_DEFAULT_CAPACITY;
        // Make sure empty string of len 0
        buff->content[0] = 0;
        return buff;
    }else{
        exit(1);
    }
}

void buff_remove(HEBuff* buff){
    if(buff->content != NULL){
        free(buff->content);
    }
    if(buff != NULL){
        free(buff);
    }
}

// Sets the length to 0 in order to start appending data from the beggining of
// the buffer again. The data is not cleared, only the first byte set to 0
void buff_clear_dirty(HEBuff* buff){
    buff->content[0] = 0;
    buff->len = 0;
}

// Sets the length to 0 in order to start appending data from the beggining of
// the buffer again. The capacity will stay the same and the buffer content
// pointer will be the same also. The old data is set to 0
void buff_clear(HEBuff* buff){
    //memset(buff->content, 0, buff->len + 1);
    //buff->len = 0;
    buff_clear_dirty(buff);
}

void buff_delete_last(HEBuff* buff){
    if(buff->len > 0 ){
        buff->content[buff->len-1] = 0;
        buff->len--;
    }
}

// Appends `to_append` to the HEBuff content and reallocates in case is needed
void buff_append(HEBuff* buff, const char* to_append, size_t len){

    // If the HEBuff len exceeds the capacity, reallocate and double the
    // capacity
    if( ( buff->len + len + 1 ) >= buff->capacity ){
        buff->capacity += len; // Increment the capacity to fit the data
        buff->capacity *= 2; // Double the size of the capacity
        buff->content = realloc(buff->content, buff->capacity);
        if ( buff->content == NULL ){
            perror("Unable to realloc HEBuff content");
            exit(1);
        }
    }

    memcpy(buff->content + buff->len, to_append, len);
    buff->len += len;
    // Add null byte at the end of memcpy
    buff->content[buff->len] = 0;
}

void buff_vappendf(HEBuff* buff, const char* fmt, ...){

    char buffer[HEBUFF_DEFAULT_CAPACITY];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buffer, HEBUFF_DEFAULT_CAPACITY, fmt, ap);
    va_end(ap);

    buff_append(buff, buffer, len);
}
