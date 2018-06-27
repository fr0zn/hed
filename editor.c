#include "editor.h"
#include "term.h"
#include "utils.h"

static HEState *hestate = NULL;

void editor_init( char *filename){
    hestate = malloc(sizeof(HEState));
    // Gets the terminal size
    term_get_size(hestate);
    // Enter in raw mode
    term_enable_raw(hestate);
    // Initialize the screen buffer
    hestate->buff = buff_create();
    // ... file open ...
    // ........
    term_clear();
    // Register the exit statement
    atexit(editor_exit);
}

// Clears all buffers and exits the editor
void editor_exit(){
    if(hestate != NULL){
        // Free the screen buff
        buff_remove(hestate->buff);
        if (hestate->file_content != NULL){
            // Free the read file content
            free(hestate->file_content);
        }
        if (hestate->file_name != NULL){
            // Free the filename
            free(hestate->file_name);
        }

        // Clear the screen
        term_clear();

        // Exit raw mode
        term_disable_raw(hestate);

        free(hestate);
    }
}

void editor_refresh_screen(){


    HEBuff* buff = hestate->buff;

    buff_append(buff, "HI", 2);

    term_clear();
    term_draw(buff);
    buff_clear(buff);
}

// Process the key pressed
void editor_process_keypress(){

    int c = utils_read_key();
    switch (c){
        case 'q':
            exit(0);
            break;
    }

}
