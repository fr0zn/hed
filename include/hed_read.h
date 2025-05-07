#ifndef HED_READ_H
#define HED_READ_H

#include <hed_buff.h>

/**
 * @brief      Reads a key pressed by the user.
 *
 * @return     Returns a value from the enum KEY_CODE or the corresponding
 *             character or number
 * 			   (check `include/hed_types.h` `KEY_CODE`)
 */
KEY_CODE read_key(void);

/**
 * @brief      Reads a line.
 *
 * Reads input from the user until KEY_ENTER is pressed.
 * If either Ctrl+C or ESC are pressed the read terminates with an
 * empty string. A prompt can be used to read the line.
 *
 * Example:
 *             HEDBuff *buff = buff_create();
 *
 *             read_line(buff,"")  // No prompt
 *             <user typing>
 *             printf("%s", buff); // Outputs: <user_typing>
 *
 *             read_line(buff,"$") // Shell like prompt
 *             $<user typing>
 *             printf("%s", buff); // Outputs: <user_typing>
 *
 *             buff_remove(buff);
 *
 * @param      buff    The buffer where the data will be read
 *             (see `include/hed_buff.h`)
 * @param      prompt  The prompt used to read the input
 *
 * @return     Rumber of bytes read
 */
int read_line(HEDBuff* buff, const char *prompt);

#endif
