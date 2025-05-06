#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include <hed_utils.h>
#include <hed_buff.h>

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


void utils_hexstring_to_buff(HEDBuff* hexstr, HEDBuff* bytearray) {

    buff_clear(bytearray);

    unsigned char raw_val;
    char val;

    for (size_t count = 0; count < hexstr->len; count+=2) {
        sscanf(&hexstr->content[count], "%2hhx", &raw_val);
        raw_val = raw_val & 0xff;
        // cast unsigned input to char - modern compilers will optimize this
        // away to a no-op
        val = raw_val;
        buff_append(bytearray, &val, 1);
    }
}

bool utils_hexonly(HEDBuff* hexstr) {

    for (int i = 0; i < hexstr->len; i++) {
        if (!isxdigit(hexstr->content[i])) {
            return false;
        }
    }

    return true;
}