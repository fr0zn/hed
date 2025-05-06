#ifndef HED_EDITOR_H
#define HED_EDITOR_H

#include <termios.h>
#include <stdbool.h>

#include <hed_buff.h>
#include <hed_utils.h>
#include <hed_action.h>
#include <hed_grammar.h>
#include <hed_search.h>

#define EDITOR_COMMAND_STATUS_OFFSET_Y 0  // Offset from screen rows (MAX)
#define EDITOR_COMMAND_STATUS_OFFSET_X 10 // Offset from screen cols (MAX)

#define EDITOR_RULER_OFFSET_Y 1
#define EDITOR_RULER_RIGHT_OFFSET_X 25

enum editor_mode {
    MODE_NORMAL       = 0x001,
    MODE_COMMAND      = 0x002,
    MODE_REPLACE      = 0x003,
    MODE_INSERT       = 0x004,
    MODE_APPEND       = 0x005,
    MODE_CURSOR       = 0x006,
    MODE_VISUAL       = 0x007,
    MODE_GRAMMAR      = 0x008,
};

enum status_message {
    STATUS_INFO       = 0x001,
    STATUS_WARNING    = 0x002,
    STATUS_ERROR      = 0x003,
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

    HEDBuff *buff; // Store the screen content

    enum editor_mode mode; // Mode the editor is in
    HEDBuff* status_message; // Message in the status bar

    unsigned int bytes_per_line;

    int scrolled; // Lines scrolled

    HESelection selection; // For visual mode

    HEActionList *action_list;

    bool in_ascii;

    HEDBuff *read_buff; // For command
    HEDBuff *search_buff; // For search command
    HEDFound last_found;

    HEGrammarList *grammars;

    // Stores the next command repetitions (from normal mode)
    HEDBuff *repeat_buff;
    unsigned int repeat;

    // Write
    byte_t last_byte;
    unsigned int last_write_offset;

    HEDByte *content;
    bool read_only;
    bool warned_read_only;

    // If the init_msg was shown
    bool init_msg;

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
void editor_init();

// Clears all buffers and exits the editor
void editor_exit();

#endif
