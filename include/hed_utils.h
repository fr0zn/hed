#ifndef HED_UTILS_H
#define HED_UTILS_H

#include <stdint.h>

/**
 * @brief      Converts an hexadecimal ascii character to the corresponding
 *             value in decimal
 *
 * Examples:
 *             utils_hex2int('a') will return 10 (0xa)
 *             utils_hex2int('A') will return 10 (0xa)
 *             utils_hex2int('F') will return 15 (0xf)
 *             utils_hex2int('R') will return 0  (0x0) // Note no valid hex
 *             utils_hex2int('!') will return 0  (0x0) // Note no valid hex
 *
 * @param      chr   The character to convert
 *
 * @return     Integer representing the value of the parameter, 0 in case it
 *             is not a valid hex value (See Examples)
 */
int utils_hex2int(const char chr);

#endif
