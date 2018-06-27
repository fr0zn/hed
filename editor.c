#include "editor.h"
#include "term.h"
#include "utils.h"

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <string.h>

static HEState *hestate = NULL;
// Instance as I
#define I hestate

#define LEN_OFFSET 9
#define PADDING 1

// Opens the file and stores the content
void editor_open_file(char *filename){
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL){

    }

    struct stat statbuf;
    if(stat(filename, &statbuf) == -1){
        perror("Cannot stat file");
        exit(1);
    }

    I->file_content = malloc(statbuf.st_size);
    I->file_name = malloc(strlen(filename));
    I->content_length = statbuf.st_size;

    strcpy(I->file_name, filename);

    if(fread(I->file_content, 1, statbuf.st_size, fp) < (size_t) statbuf.st_size){
        perror("Unable to read file content");
        free(I->file_content);
        exit(1);
    }
}


void editor_render_ascii(HEBuff* buff, char *asc, unsigned int len){
    for(int c = 0; c < len; c++){
        buff_vappendf(buff, "%c", asc[c]);
    }
}

void editor_render_content(HEBuff* buff){
    // TODO: fill the screen
    int row = 0;
    int col = 0;

    char *asc;

    int offset_start = 0;
    int offset_end = 0;
    int offset;

    int hex_len = I->screen_cols - 10;

    // Calculate the maximum hex+ascii bytes that fits in the screen size
    // one byte = 2 chars
    int bytes_per_line = I->bytes_group; //
    int filled;

    do
    {
        bytes_per_line += I->bytes_group;
        filled = 10 + bytes_per_line * 2
            + (bytes_per_line/I->bytes_group) * PADDING + bytes_per_line;

    }while(filled <= I->screen_cols);

    /*bytes_per_line -= I->bytes_group;*/

    // one byte is 2 octets
    int groups_per_line = ((hex_len)/(2*2*I->bytes_group + PADDING) );
    int hex_per_line = groups_per_line * 3; // 2 octets per group + spaces

    // Not bigger than the settings
    if (groups_per_line > I->groups_per_line){
        groups_per_line = I->groups_per_line;
    }

    // Ascii
    asc = malloc(bytes_per_line);

    for (offset = 0; offset < I->content_length; offset++){
        if(offset % bytes_per_line == 0 ){
            buff_vappendf(buff, "%08x: ", offset);
        }

        // Store the value to asc buffer
        asc[offset%bytes_per_line] = I->file_content[offset];

        // Write the value on the screen (HEBuff)
        buff_vappendf(buff, "%02x", I->file_content[offset]);

        // Every group, write a separator of len PADDING
        if(offset % I->bytes_group){
            for(int s=0; s < PADDING; s++){
                buff_append(buff, " ", 1);
            }
        }
        // If end of line, write ascii and new line
        if((offset + 1) % bytes_per_line == 0){
            editor_render_ascii(buff, asc, bytes_per_line);
            buff_append(buff, "\r\n", 2);
        }
    }

    int leftover = offset % (I->screen_cols - bytes_per_line);
    if (leftover > 0){
        for(int i=0; i< leftover; i++){
            buff_append(buff, " ", 1);
        }
        /*editor_render_ascii(buff, asc, bytes_per_line);*/
    }

    free(asc);

    // padding chars

}

void editor_refresh_screen(){

    HEBuff* buff = I->buff;

    // Clear the content of the buffer and write the new
    buff_clear(buff);
    editor_render_content(buff);

    // Clear the screen and write the buffer on it
    term_clear();
    term_draw(buff);
}

// Process the key pressed
void editor_process_keypress(){

    int c = utils_read_key();
    switch (c){
        case 'q':
            exit(0);
            break;
    }

}

void editor_resize(){

    // Get new terminal size
    term_get_size(I);
    // Redraw the screen
    editor_refresh_screen();
}

void editor_init(char *filename){
    I = malloc(sizeof(HEState));
    // Gets the terminal size
    term_get_size(I);
    // Enter in raw mode
    term_enable_raw(I);
    // Initialize the screen buffer
    I->buff = buff_create();

    // Set HEState variables
    I->bytes_group = 2;
    I->groups_per_line = 40;

    editor_open_file(filename);
    // ... file open ...
    // ........
    I->file_name = NULL;

    term_clear();
    // Register the resize handler
    signal(SIGWINCH, editor_resize);
    // Register the exit statement
    atexit(editor_exit);
}

// Clears all buffers and exits the editor
void editor_exit(){
    if(I != NULL){
        // Free the screen buff
        buff_remove(I->buff);
        if (I->file_content != NULL){
            // Free the read file content
            free(I->file_content);
        }
        if (I->file_name != NULL){
            // Free the filename
            free(I->file_name);
        }

        // Clear the screen
        term_clear();

        // Exit raw mode
        term_disable_raw(I);

        free(I);
    }
}
