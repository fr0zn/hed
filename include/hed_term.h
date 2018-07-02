#ifndef HED_TERM_H
#define HED_TERM_H

#include <unistd.h>

#include <hed_editor.h>
#include <hed_buff.h>

/**
 * @brief      Gets the current terminal size
 *
 * Gets the current terminal size (rows and columns) and stores the value in
 * the corresponding argument parameter
 *
 * @param      rows  Will contain the number of rows in the current terminal
 * @param      cols  Will contain the number of columns in the current terminal
 */
void term_get_size(int* rows, int* cols);

/**
 * @brief      Prints data to the screen
 *
 * Prints data of length `len` to the terminal
 *
 * @param  	   data  Pointer to the data to be written
 * @param      len   The length of the data
 */
void term_print_data(const char *data, ssize_t len);

// Enables terminal raw mode

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
void term_clear_screen();

// Prints a char array

#endif
