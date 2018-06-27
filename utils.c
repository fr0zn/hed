#include "utils.h"
#include <unistd.h>

int utils_read_key(){
    char c;
    ssize_t nread;
    while ((nread = read(STDIN_FILENO, &c, 1)) == 0);
    if (nread == -1){
        return -1;
    }

    return c;
}
