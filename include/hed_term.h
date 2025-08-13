#ifndef HED_TERM_H
#define HED_TERM_H

#include <unistd.h>

#include <hed_editor.h>
#include <hed_buff.h>

#define TERM_CURSOR_HIDE "\x1b[?25l"
#define TERM_CURSOR_HIDE_LEN 6

#define TERM_CURSOR_SHOW "\x1b[?25h"
#define TERM_CURSOR_SHOW_LEN 6

#define TERM_CLEAR_LINE "\x1b[2K"
#define TERM_CLEAR_LINE_LEN 4

#define TERM_CLEAR_LINE_END "\x1b[0K"
#define TERM_CLEAR_LINE_END_LEN 4

typedef enum {
    FORMAT_RESET = 0,
    FORMAT_BOLD,
    FORMAT_DIM,
    FORMAT_UNDERLINE = 4,
    FORMAT_BLINK,
    FORMAT_INVERTED = 7,
    FORMAT_HIDDEN,
} TERM_FORMAT;

typedef enum {
    FG_DEFAULT = 39,
    FG_BLACK = 30,
    FG_RED,
    FG_GREEN,
    FG_YELLOW,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_LIGHT_GRAY,
    FG_DARK_GRAY = 90,
    FG_LIGHT_RED,
    FG_LIGHT_GREEN,
    FG_LIGHT_YELLOW,
    FG_LIGHT_BLUE,
    FG_LIGHT_MAGENTA,
    FG_LIGHT_CYAN,
    FG_WHITE,
} TERM_COLOR_FG;

typedef enum {
    BG_DEFAULT = 49,
    BG_BLACK = 40,
    BG_RED,
    BG_GREEN,
    BG_YELLOW,
    BG_BLUE,
    BG_MAGENTA,
    BG_CYAN,
    BG_LIGHT_GRAY,
    BG_DARK_GRAY = 100,
    BG_LIGHT_RED,
    BG_LIGHT_GREEN,
    BG_LIGHT_YELLOW,
    BG_LIGHT_BLUE,
    BG_LIGHT_MAGENTA,
    BG_LIGHT_CYAN,
    BG_WHITE,
} TERM_COLOR_BG;

/**
 * @brief      Gets the current terminal size
 *
 * Gets the current terminal size (rows and columns) and stores the value in
 * the corresponding argument parameter
 *
 * @param      rows  Will contain the number of rows in the current terminal
 * @param      cols  Will contain the number of columns in the current terminal
 */
void term_get_size(unsigned int* rows, unsigned int* cols);

/**
 * @brief      Prints data to the screen
 *
 * Prints data of length `len` to the terminal
 *
 * @param      data  Pointer to the data to be written
 * @param      len   The length of the data
 */
void term_print(const char *data, ssize_t len);

/**
 * @brief      Sets the color or format on the terminal but instead of
 *             printing the corresponding escape sequence appends the data
 *             to a HEDBuff (see `include/hed_buff.h`)
 *
 * @param      buff   The buffer
 * @param      format_color  The color or format to set the terminal to.
 *             Check TERM_COLOR_FG, TERM_COLOR_BG and TERM_FORMAT
 *
 * @return     Number of characters needed for the escape sequence
 */
int term_set_format_buff(HEDBuff* buff, int format_color);

/**
 * @brief      Sets the color or format on the terminal
 *
 * @param      format_color  The color or format to set the terminal to.
 *             Check TERM_COLOR_FG, TERM_COLOR_BG and TERM_FORMAT
 *
 * @return     Number of characters needed for the escape sequence
 */
int term_set_format(int format_color);

/**
 * @brief      Hides the cursor from the terminal screen
 */
void term_cursor_hide(void);

/**
 * @brief      Shows the cursor from the terminal screen
 */
void term_cursor_show(void);

/**
 * @brief      Moves the cursor to position (x, y)
 *
 * @param      row   Row to move to
 * @param      col   Column to move to
 */
void term_goto(unsigned int row, unsigned int col);

/**
 * @brief      Moves the cursor to position (x, y) but instead of printing
 *             the corresponding escape sequence appends the data to a
 *             HEDBuff (see `include/hed_buff.h`)
 *
 * @param      row   Row to move to
 * @param      col   Column to move to
 */
void term_goto_buff(HEDBuff* buff, unsigned int row, unsigned int col);

/**
 * @brief      Clears the terminal current line but instead of printing
 *             the corresponding escape sequence appends the data to a
 *             HEDBuff (see `include/hed_buff.h`)
 *
 * @param      buff  The buffer
 */
void term_clear_line_buff(HEDBuff* buff);

/**
 * @brief      Clears the terminal from the current position until the end of
 *             the current line
 */
void term_clear_line_end(void);

/**
 * @brief      Clears the terminal from the current position until the end of
 *             the current line but instead of printing the corresponding
 *             escape sequence appends the data to a
 *             HEDBuff (see `include/hed_buff.h`)
 *
 * @param      buff  The buffer
 */
void term_clear_line_end_buff(HEDBuff* buff);

/**
 * @brief      Enables raw mode for the current terminal
 *
 * Enables raw mode for the current terminal and stores the current terminal
 * information to the struct termios `term_original` given as a parameter.
 *
 * @param      term_original  Termios struct that will contain the original
 * information of the terminal after setting the raw mode.
 */
void term_enable_raw(struct termios* term_original);

// Disable terminal raw mode

/**
 * @brief      Disables raw mode for the current terminal
 *
 * Disables raw mode for the current terminal and restores the current terminal
 * with the information of the struct termios `term_original` given as a
 * parameter.
 *
 * @param      term_original  Termios struct that contains the original
 * information of the terminal without raw mode.
 */
void term_disable_raw(struct termios* term_original);

// Clear screen

/**
 * @brief      Clears the terminal screen
 *
 * Clears the terminal by reseting the colors, moving to the top-left
 * of the screen and clearing it until the end
 *
 */
void term_clear_screen(void);

// Prints a char array

#endif
