#ifndef HE_EDITOR_H
#define HE_EDITOR_H

#include "buff.h"
#include <termios.h>

// Struct to store the state of the editor, such as the position on the screen
// the file content and length and the status message. It also holds the
// original terminal settings to restore on exit
typedef struct {

    int screen_rows; // Number of rows of the screen
    int screen_cols; // Number of cols of the screen

    int cursor_x;
    int cursor_y;

    HEBuff *buff; // Store the screen content

    char *file_content;  // The content of the file we are working on
    char *file_name; // The name of the file we are working on
    unsigned int content_length; // Stores the length of the file

    struct termios term_original; // Stores the non raw terminal info

}HEState;

// Initialize the editor state
void editor_init(char *filename);

// Clears all buffers and exits the editor
void editor_exit();

// Redraws the screen content
void editor_refresh_screen();

// Process the key pressed
void editor_process_keypress();

#endif
