#ifndef HE_EDITOR_H
#define HE_EDITOR_H

#include "buff.h"
#include "action.h"
#include <termios.h>
#include <stdbool.h>

enum editor_mode {
    MODE_NORMAL       = 0x001,
    MODE_COMMAND      = 0x002,
    MODE_REPLACE      = 0x003,
    MODE_INSERT       = 0x004,
    MODE_CURSOR       = 0x005,
};

typedef struct {
    uint8_t o; // Original
    uint8_t c; // Current
} byte_t;

// Struct to store the state of the editor, such as the position on the screen
// the file content and length and the status message. It also holds the
// original terminal settings to restore on exit
typedef struct {

    int screen_rows; // Number of rows of the screen
    int screen_cols; // Number of cols of the screen

    int cursor_x;
    int cursor_y;

    HEBuff *buff; // Store the screen content

    enum editor_mode mode; // Mode the editor is in
    char* status_message; // Pointer to the message in the status bar

    unsigned int bytes_per_line;
    unsigned int bytes_group;
    unsigned int groups_per_line;

    HEActionList *action_list;

    bool in_ascii;

    // Stores the next command repetitions (from normal mode)
    HEBuff *repeat_buff;
    unsigned int repeat;

    // Write
    unsigned char last_byte;
    unsigned int last_write_offset;

    //uint8_t *bytes_changed; // Map keeping track of the bytes changed (1->changed, 0->original)

    byte_t *content;

    //char *original_content;  // The original content of the file we are working on
    //char *file_content;  // The content of the file we are working on
    char *file_name; // The name of the file we are working on
    unsigned int content_length; // Stores the length of the file

    struct termios term_original; // Stores the non raw terminal info

}HEState;

// Opens the file and stores the content
void editor_open_file(char *filename);

// Redraws the screen content
void editor_refresh_screen();

// Process the key pressed
void editor_process_keypress();

// SIGWINCH handler
void editor_resize();

// Initialize the editor state
void editor_init(char *filename);

// Clears all buffers and exits the editor
void editor_exit();

#endif
