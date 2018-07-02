#include <unistd.h>

#include <hed_types.h>
#include <hed_utils.h>

int utils_read_key(){
    char c;
    ssize_t nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) == 0);
    if (nread == -1){
        return -1;
    }

    char seq[4]; // escape sequence buffer.

    switch(c){
        case KEY_ESC:
            if (read(STDIN_FILENO, seq, 1) == 0) {
                return KEY_ESC;
            }
            if (read(STDIN_FILENO, seq + 1, 1) == 0) {
                return KEY_ESC;
            }
            if (seq[0] == '[') {
                switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
                }
            }
    }

    return c;
}

unsigned char utils_atoh (unsigned char chr){

    if (chr > '9'){
        chr += 9;
    }

    return (chr &= 0x0F);
}
