#include <hed_utils.h>

int utils_hex2int(const char chr) {
   
    // Check if its a number
    if ('0' <= chr && chr <= '9') {
        return chr - '0';
    // Check if its a lowercase valid hex
    } else if ('a' <= chr && chr <= 'f') {
        return 10 + chr - 'a';
    // Check if its an uppercase valid hex
    } else if ('A' <= chr && chr <= 'F') {
        return 10 + chr - 'A';
    }

    // Make sure we return 0 if is not a valid hex
    return 0;
}
