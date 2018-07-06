#include <stdio.h>

#include <hed_editor.h>

int main(int argc, char *argv[]){

    editor_init(argv[1]);

    if(argc > 1){
        editor_open_file(argv[1]);
    }

    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

