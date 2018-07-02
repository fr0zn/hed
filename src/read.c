#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <hed_read.h>
#include <hed_buff.h>
#include <hed_term.h>
#include <hed_types.h>

// Buffer for the readline
static HEBuff *buff = NULL;

// Returns a key representation from the KEY_CODE enum by performing escape
// sequence parsing. Several keys exist that have a direct key representation,
// such as letters and numbers. Other keys requires parsing the escape sequence
// '\x1b' (KEY_ESC) followed by '['. 
// 
// The values for an specific keyboard can be optained with the 
// python script `helpers/key_code.py`
//
KEY_CODE read_key(){
    char c;
    ssize_t nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) == 0);
    if (nread == -1){
        return -1;
    }

    char seq[4]; // escape sequence buffer.

    switch(c){
        case KEY_ESC:
            if (read(STDIN_FILENO, seq, 1) == 0) {
                return KEY_ESC;
            }
            if (read(STDIN_FILENO, seq + 1, 1) == 0) {
                return KEY_ESC;
            }
            if (seq[0] == '[') {
                switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                }
            }
    }

    return c;
}

char* read_line(const char *prompt){

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
        c = read_key();

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
