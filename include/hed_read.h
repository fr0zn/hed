#ifndef HED_READ_H
#define HED_READ_H

#include <hed_types.h>

/**
 * @brief      Reads a key pressed by the user.
 *
 * @return     Returns a value from the enum KEY_CODE or the corresponding
 *             character or number
 * 			   (check `include/hed_types.h` `KEY_CODE`)
 */
KEY_CODE read_key();

/**
 * @brief      Reads a line.
 * 
 * Reads input from the user until KEY_ENTER is pressed.
 * If either Ctrl+C or ESC are pressed the read terminates with an 
 * empty string. A prompt can be used to read the line.
 * 
 * Example:
 * 			   read_line("")  // No prompt
 * 			   <user typing>
 * 			   read_line("$") // Shell like prompt
 * 			   $<user typing>
 *
 * @param[in]  prompt  The prompt used to read the input
 *
 * @return     A pointer to the typed line null terminated
 */
char* read_line(const char *prompt);

#endif
