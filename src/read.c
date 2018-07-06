#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <hed_read.h>
#include <hed_buff.h>
#include <hed_term.h>
#include <hed_types.h>

// Returns a key representation from the KEY_CODE enum by performing escape
// sequence parsing. Several keys exist that have a direct key representation,
// such as letters and numbers. Other keys requires parsing the escape sequence
// '\x1b' (KEY_ESC) followed by '['.
//
// The values for an specific keyboard can be optained with the
// python script `helpers/key_code.py`
//
KEY_CODE read_key() {
    char c;
    ssize_t nread;

    // Read until we get the byte
    while ((nread = read(STDIN_FILENO, &c, 1)) == 0);
    // In case of error
    if (nread == -1) {
        return KEY_NULL;
    }

    // This buffer will contain the escaped sequence
    char seq[3];

    switch(c){
        // If it starts with ESC, try to read more bytes, if the read fails,
        // a pure ESC key has been pressed
        case KEY_ESC: {
            // Read first byte of the sequence, it should be a '['
            if (read(STDIN_FILENO, &seq[0], 1) == 0) {
                return KEY_ESC;
            }
            // Read the seond byte of the sequence
            if (read(STDIN_FILENO, &seq[1], 1) == 0) {
                return KEY_ESC;
            }
            // If the sequence starts with '['
            if (seq[0] == '[') {
                // Up until here, the full sequence is ESC[ (0x1b 0x5b)
                // There exists some keys, like F1-12 and HOME, END ...
                // that the sequence follows a number + the character '~'.
                // Try to read that character '~' previously checking
                // for the number
                if (seq[1] >= '0' && seq[1] <= '9') {
                    if (seq[2] == '~') {
                        switch (seq[1]) {
                            case '1': return KEY_HOME;     // ESC[1~
                            case '3': return KEY_DEL;      // ESC[3~
                            case '4': return KEY_END;      // ESC[4~
                            case '5': return KEY_PAGEUP;   // ESC[5~
                            case '6': return KEY_PAGEDOWN; // ESC[6~
                            default: return KEY_NULL;      // Not implemented
                        }
                    }
                }else{
                    switch (seq[1]) {
                        case 'A': return KEY_UP;    // ESC[A
                        case 'B': return KEY_DOWN;  // ESC[B
                        case 'C': return KEY_RIGHT; // ESC[C
                        case 'D': return KEY_LEFT;  // ESC[D
                        default: return KEY_NULL;   // Not implemented
                    }
                }
            }
        }
    }

    // Return the read character/number
    return c;
}

// The data will be stored inside the HEDBuff `buff` object
// (check `include/hed_buff.h`)
int read_line(HEDBuff* buff, const char *prompt){

    char c = 0;

    // Clear the buffer so we start fresh
    buff_clear_dirty(buff);

    term_cursor_show();

    // Print the prompt
    term_print(prompt, strlen(prompt));

    // Read until we don't press ENTER
    while(c != KEY_ENTER){
        // Get the pressed key
        c = read_key();

        // If we pressed the backspace, we have to remove the last
        // character from the buffer, and move the cursor one position left
        // cleaning that character. This is done by moving one character left,
        // writting an space ' ' and moving the cursor back again.
        if(c == KEY_BACKSPACE){
            //
            if(buff->len > 0){
                term_print("\x1b[1D",4);
                term_print(" ",1);
                term_print("\x1b[1D",4);
                // Delete last character from the buffer
                buff_delete_last(buff);
            }
            continue;
        }
        // If we press the ESC or Ctrl+C, clear the buffer and break the loop
        if(c == KEY_CTRL_C || c == KEY_ESC){
            buff_clear_dirty(buff);
            break;
        }
        // If its a printable byte, character, symbol, number. Add the char to
        // the buffer and show it on the screen
        if(isprint(c) && c != KEY_NULL){
            buff_append(buff, &c, 1);
            term_print(&c,1);
        }
    }

    term_cursor_hide();

    return buff->len;
}
