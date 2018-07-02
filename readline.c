#include "readline.h"
#include "buff.h"
#include "utils.h"
#include "term.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Buffer for the readline
static HEBuff *buff = NULL;

char* readline(const char *prompt ){

    char c = 0;

    // If we don't have a buffer, create it. Clear it instead
    if(buff == NULL){
        buff = buff_create();
    }else{
        buff_clear_dirty(buff);
    }

    // Print the prompt
    term_print_data(prompt, strlen(prompt));

    while(c != KEY_ENTER){
        // Read 1 byte
        c = utils_read_key();

        if(c == KEY_BACKSPACE){
            if(buff->len > 0){
                buff_delete_last(buff);
                // Move back, writting an space
                term_print_data("\x1b[1D",4);
                term_print_data(" ",1);
                term_print_data("\x1b[1D",4);
            }
            continue;
        }
        if(c == KEY_CTRL_C || c == KEY_ESC){
            // Clear the buffer
            buff_clear_dirty(buff);
            break;
        }
        if(isprint(c)){
            buff_append(buff, &c, 1);
            term_print_data(&c,1);
        }
    }

    return buff->content;

}
