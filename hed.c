#include "hed_editor.h"
#include <stdio.h>

int main(int argc, char *argv[]){

    if(argc < 2){
        // start empty file
        printf("Give file to open");
        return 1;
    }

    editor_init(argv[1]);

    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

