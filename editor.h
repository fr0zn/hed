#ifndef HE_EDITOR_H
#define HE_EDITOR_H

#include "buff.h"
#include "utils.h"
#include "action.h"
#include "grammar.h"
#include <termios.h>
#include <stdbool.h>

enum editor_mode {
    MODE_NORMAL       = 0x001,
    MODE_COMMAND      = 0x002,
    MODE_REPLACE      = 0x003,
    MODE_INSERT       = 0x004,
    MODE_CURSOR       = 0x005,
    MODE_VISUAL       = 0x006,
    MODE_GRAMMAR      = 0x007,
};

enum status_message {
    STATUS_INFO       = 0x001,
    STATUS_WARNING    = 0x002,
    STATUS_ERROR      = 0x003,
    STATUS_MODE       = 0x004,
};

typedef struct {
    int start;
    int end;
    bool is_backwards;
}HESelection;


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
    HEBuff* status_message; // Message in the status bar

    unsigned int bytes_per_line;
    unsigned int bytes_group;
    unsigned int groups_per_line;

    int scrolled; // Lines scrolled

    HESelection selection; // For visual mode

    HEActionList *action_list;

    bool in_ascii;

    HEBuff *read_buff; // For command

    HEGrammarList *grammars;

    // Stores the next command repetitions (from normal mode)
    HEBuff *repeat_buff;
    unsigned int repeat;

    // Write
    byte_t last_byte;
    unsigned int last_write_offset;

    byte_t *content;
    bool read_only;

    bool dirty; // If changes made

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
