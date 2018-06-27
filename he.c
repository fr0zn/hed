#include "editor.h"
#include <stdio.h>

int main(int argc, char *argv[]){

    editor_init("testfile.txt");
    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

