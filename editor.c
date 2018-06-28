#include "editor.h"
#include "term.h"
#include "utils.h"

#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

static HEState *hestate = NULL;
// Instance as I
#define I hestate

#define LEN_OFFSET 9
#define PADDING 1
#define MAX_COMMAND 32
#define MAX_STATUS_BAR 128

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


void editor_render_ascii(HEBuff* buff, int row, char *asc, unsigned int len){
    char c;
    for(int i = 0; i < len; i++){
        c = asc[i];

        // Cursor
        if(I->cursor_y == row){
            if(I->cursor_x == i){
                buff_append(buff, "\x1b[7m", 4);
            }
        }

        if(isprint(c)){
            buff_vappendf(buff, "%c", c);
        }else{
            buff_append(buff, ".", 1);
        }

        buff_append(buff, "\x1b[0m", 4);
    }
}

void editor_calculate_bytes_per_line(){

    // Calculate the maximum hex+ascii bytes that fits in the screen size
    // one byte = 2 chars
    int bytes_per_line = I->bytes_group; // Atleast write one byte group
    int filled;

    while((bytes_per_line / I->bytes_group) < I->groups_per_line)
    {
        bytes_per_line += I->bytes_group;
        filled = 10 + bytes_per_line * 2
            + (bytes_per_line/I->bytes_group) * PADDING + bytes_per_line;
        if (filled >= I->screen_cols){
            bytes_per_line -= I->bytes_group;
            break;
        }

    }

    I->bytes_per_line = bytes_per_line;
}

unsigned int editor_offset_at_cursor(){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    unsigned int offset = (I->cursor_y - 1) * (I->bytes_per_line) + (I->cursor_x);
    if (offset <= 0) {
        return 0;
    }
    if (offset >= I->content_length) {
        return I->content_length - 1;
    }
    return offset;
}

void editor_cursor_at_offset(unsigned int offset){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    I->cursor_x = offset % I->bytes_per_line;
    I->cursor_y = offset / I->bytes_per_line + 1;
}

void editor_move_cursor(int dir, int amount){
    for(int i = 0; i < amount; i++){
        switch (dir){
            case KEY_UP: I->cursor_y--; break;
            case KEY_DOWN: I->cursor_y++; break;
            case KEY_LEFT: I->cursor_x--; break;
            case KEY_RIGHT: I->cursor_x++; break;
        }

        // Stop top limit
        if(I->cursor_y <= 1){
            I->cursor_y = 1;
        }

        // Stop left limit
        if(I->cursor_x < 0){
            // If not the first line, move up
            if(I->cursor_y > 1){
                I->cursor_y--;
                I->cursor_x = I->bytes_per_line-1;
            }else{
                I->cursor_x = 0;
            }
        }

        // Stop right limit
        if(I->cursor_x >= I->bytes_per_line){
            I->cursor_x = 0;
            I->cursor_y++;
        }

        unsigned int offset = editor_offset_at_cursor();

        if (offset >= I->content_length - 1) {
            editor_cursor_at_offset(offset);
        }
    }

}

void editor_render_content(HEBuff* buff){

    char *asc;
    unsigned int line_chars = 0;
    unsigned int line_bytes = 0;
    unsigned int chars_until_ascii = 0;

    int row = 0;
    int col = 0;

    int offset;

    editor_calculate_bytes_per_line();

    chars_until_ascii = 10 + I->bytes_per_line * 2 + I->bytes_per_line/2;

    // Ascii
    asc = malloc(I->bytes_per_line);

    for (offset = 0; offset < I->content_length; offset++){
        // New row
        if(offset % I->bytes_per_line == 0 ){
            line_chars = 0;
            line_bytes = 0;
            buff_vappendf(buff, "%08x: ", offset);
            line_chars += 10;
            row++;
        }

        // Cursor
        if(I->cursor_y == row){
            if(I->cursor_x == line_bytes){
                buff_append(buff, "\x1b[7m", 4);
            }
        }

        // Store the value to asc buffer
        asc[offset % I->bytes_per_line] = I->file_content[offset];

        // Write the value on the screen (HEBuff)
        buff_vappendf(buff, "%02x", (unsigned int) I->file_content[offset] & 0xff);
        buff_append(buff, "\x1b[0m", 4);
        line_chars += 2;
        line_bytes += 1;

        // Every group, write a separator of len PADDING
        if(offset % I->bytes_group){
            for(int s=0; s < PADDING; s++){
                buff_append(buff, " ", 1);
                line_chars += 1;
            }
        }
        // If end of line, write ascii and new line
        if((offset + 1) % I->bytes_per_line == 0){
            editor_render_ascii(buff, row, asc, I->bytes_per_line);
            line_chars += I->bytes_per_line; // ascii chars
            buff_append(buff, "\r\n", 2);
        }
    }

    while( (chars_until_ascii - line_chars) != 0){
            buff_append(buff, " ", 1);
            line_chars++;
    }

    editor_render_ascii(buff, row, asc, line_bytes);

    free(asc);

    // padding chars

}

void editor_goto(unsigned int x, unsigned int y){
    char buffer[32];
    int w = sprintf(buffer, "\x1b[%d;%dH", y, x);
    term_print(buffer, w);
}

void editor_render_command(char *command){
    char buffer[32];
    editor_goto(I->screen_cols-10, I->screen_rows);
    int w = sprintf(buffer, "%s", command);
    term_print(buffer, w);
}

void editor_set_status(const char *fmt, ...){

    va_list ap;
    va_start(ap, fmt);
    int x = vsnprintf(I->status_message, MAX_STATUS_BAR, fmt, ap);
    va_end(ap);

}

// Modes

void editor_start_mode_normal(){
    editor_set_status("");
}

void editor_start_mode_insert(){
    editor_set_status("-- INSERT --");
    // If we have data in the repeat buffer, start from the index 0 again
    if(I->repeat_buff->len != 0){
        I->repeat_buff->len = 0;
    }
}

void editor_start_mode_cursor(){
    editor_set_status("-- CURSOR --");
}

void editor_set_mode(enum editor_mode mode){
    I->mode = mode;
    switch(I->mode){
        case MODE_NORMAL:   editor_start_mode_normal(); break;
        case MODE_INSERT:   editor_start_mode_insert(); break;
        case MODE_CURSOR:   editor_start_mode_cursor(); break;
        case MODE_COMMAND:  break;
    }
}

void editor_render_ruler(HEBuff* buff){

    // Move to (screen_cols-10,screen_rows);
    buff_vappendf(buff, "\x1b[%d;%dH", I->screen_rows, I->screen_cols-20);

    unsigned int offset = editor_offset_at_cursor();

    buff_vappendf(buff, "0x%08x (%d)", offset, offset);
}

void editor_render_status(HEBuff* buff){

    // Move to (0,screen_rows);
    buff_vappendf(buff, "\x1b[%d;0H", I->screen_rows);

    int len = strlen(I->status_message);
    if (I->screen_cols <= len) {
        len = I->screen_cols;
        buff_append(buff, I->status_message, len-3);
        buff_append(buff, "...", 3);
    }
    buff_append(buff, I->status_message, len);
}

void editor_refresh_screen(){

    HEBuff* buff = I->buff;

    // Clear the content of the buffer
    buff_clear(buff);

    buff_append(buff, "\x1b[?25l", 6); // Hide cursor
    buff_append(buff, "\x1b[H", 3); // Move cursor top left


    editor_render_content(buff);
    editor_render_status(buff);
    editor_render_ruler(buff);

    // Clear the screen and write the buffer on it
    term_clear();
    term_draw(buff);
}

void __write_debug(char *fmt, ...){

    char buff[128];
    va_list ap;
    va_start(ap, fmt);
    int x = vsnprintf(buff, 128, fmt, ap);
    va_end(ap);

    FILE *f = fopen("debug.txt", "wa");
    if (f == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    fprintf(f, "%s\n", buff);

    fclose(f);

}

void editor_prepare_write_repeat(char ch){

    if((I->repeat_buff->len+1 == I->repeat_buff->capacity)){
        I->repeat_buff->content = realloc(I->repeat_buff->content, I->repeat_buff->capacity*2);
        I->repeat_buff->capacity *= 2;
    }
    I->repeat_buff->content[I->repeat_buff->len] = ch;
    I->repeat_buff->len++;
}


void editor_write_cursor(char c){

    char new_byte = 0;
    char old_byte = 0;

    unsigned int offset = editor_offset_at_cursor();

    if(offset >= I->content_length){
        return;
    }

    if(I->last_write_offset == offset){
        // One octet already written in this position
        old_byte = I->file_content[offset];

        // Less significative first
        new_byte = ((old_byte << 4) & 0xf0) | utils_atoh(c) ;
        // Most significative first
        //new_byte = ((old_byte) & 0xf0) | utils_atoh(c) ;
        I->file_content[offset] = new_byte;

        editor_move_cursor(KEY_RIGHT, 1);

        I->last_write_offset = -1;
    }else{
        old_byte = I->file_content[offset];

        // Less significative first
        new_byte = (old_byte & 0xf0) | utils_atoh(c) ;
        // Most significative first
        //new_byte = (old_byte & 0x0f) | (utils_atoh(c) << 4) ;
        I->file_content[offset] = new_byte;

        I->last_byte = old_byte;
        I->last_write_offset = offset;
    }

}

void editor_reset_write(){
    I->last_write_offset = -1;
    //I->repeat_buff->len = 0;
}

void editor_write_cursor_repeat(){

    for(int r=0; r < I->repeat; r++){
        for(int c=0; c < I->repeat_buff->len; c++){
            editor_write_cursor(I->repeat_buff->content[c]);
        }
    }
}

void editor_undo(){

}

// Process the key pressed
void editor_process_keypress(){

    char command[MAX_COMMAND];
    command[0] = '\0';

    // Read first command char
    int c = utils_read_key();

    if(I->mode == MODE_NORMAL){
        // Reset repeat commands
        I->repeat = 1;
        // TODO: Implement key repeat correctly
        if(c != '0'){
            unsigned int count = 0;
            while(isdigit(c) && count < 5){
                command[count] = c;
                command[count+1] = '\0';
                editor_render_command(command);
                c = utils_read_key();
                count++;
            }

            // Store in case we change mode
            I->repeat = atoi(command);
            if(I->repeat <= 0 ){
                I->repeat = 1;
            }
        }

        switch (c){
            case 'q': exit(0); break;

            case 'h': editor_move_cursor(KEY_LEFT, I->repeat); break;
            case 'j': editor_move_cursor(KEY_DOWN, I->repeat); break;
            case 'k': editor_move_cursor(KEY_UP, I->repeat); break;
            case 'l': editor_move_cursor(KEY_RIGHT, I->repeat); break;

            case 'w': editor_move_cursor(KEY_RIGHT, I->repeat*I->bytes_group); break;
            case 'b': editor_move_cursor(KEY_LEFT, I->repeat*I->bytes_group); break;

            // Modes
            case KEY_ESC: editor_set_mode(MODE_NORMAL); break;
            case 'i': editor_set_mode(MODE_INSERT); break;
            case 'c': editor_set_mode(MODE_CURSOR); break;

            // Repeat last write command
            case '.': editor_write_cursor_repeat(); break;

            // EOF
            case 'G': editor_cursor_at_offset(I->content_length-1); break;
            case 'g':
                editor_render_command("g");
                c = utils_read_key();
                if(c == 'g'){
                    editor_cursor_at_offset(0);
                }
                break;
            // Start of line
            case '0': I->cursor_x = 0; break;
            // End of line
            case '$': editor_move_cursor(KEY_RIGHT, I->bytes_per_line-1 - I->cursor_x); break;
        }
    }

    else if(I->mode == MODE_INSERT){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC || c == 'q'){
            // Already one repeat write
            I->repeat--;
            editor_write_cursor_repeat();
            editor_reset_write();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_write_cursor(c);
            editor_prepare_write_repeat(c);
        }
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
    I->groups_per_line = 8;

    // status bar & mode
    I->mode = MODE_NORMAL;
    I->status_message = malloc(MAX_STATUS_BAR);

    // Write
    I->last_byte = 0;
    I->last_write_offset = -1;
    I->repeat_buff = malloc(sizeof(HEBuff));
    I->repeat_buff->content = malloc(128);
    I->repeat_buff->capacity = 128;
    I->repeat = 1;

    I->cursor_y = 1;
    I->cursor_x = 0;

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
        if (I->buff != NULL){
            // Free the screen buff
            buff_remove(I->buff);
        }
        if (I->file_content != NULL){
            // Free the read file content
            free(I->file_content);
        }
        if (I->file_name != NULL){
            // Free the filename
            free(I->file_name);
        }
        if (I->status_message != NULL){
            // Free the status_mesasge
            free(I->status_message);
        }

        // Clear the screen
        term_clear();

        // Exit raw mode
        term_disable_raw(I);

        free(I);
    }
}
