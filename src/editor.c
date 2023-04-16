#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pwd.h>

#include <hed_editor.h>
#include <hed_term.h>
#include <hed_utils.h>
#include <hed_action.h>
#include <hed_grammar.h>
#include <hed_config.h>
#include <hed_read.h>
#include <hed_search.h>


static HEState *g_hestate = NULL;
static HEDConfig* g_config;
// Instance as I
#define I g_hestate

#define LEN_OFFSET 9
#define PADDING 1
#define MAX_COMMAND 32
#define MAX_STATUS_BAR 128

void editor_set_status(enum status_message type, const char *fmt, ...){

    char buff[MAX_STATUS_BAR];

    // We control the len on print
    buff_clear_dirty(I->status_message);
    // Clear line
    term_clear_line_end_buff(I->status_message);

    switch(type){
        case STATUS_INFO:
            term_set_format_buff(I->status_message, FG_BLUE); break;
        case STATUS_WARNING:
            term_set_format_buff(I->status_message, FG_YELLOW); break;
        case STATUS_ERROR:
            term_set_format_buff(I->status_message, FG_RED); break;
    }

    va_list ap;
    va_start(ap, fmt);
    int x = vsnprintf(buff, MAX_STATUS_BAR, fmt, ap);
    va_end(ap);

    buff_append(I->status_message, buff, x);

    term_set_format_buff(I->status_message, FORMAT_RESET);

}

// Opens the file and stores the content
void editor_open_file(char *filename){
    if (filename == NULL) {
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL){

    }

    struct stat statbuf;
    if(stat(filename, &statbuf) == -1){
        editor_set_status(STATUS_ERROR, "Cannot stat file");
        return;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        editor_set_status(STATUS_ERROR, "File '%s' is not a regular file\n",
            filename);
        return;

    }

    I->init_msg = true;

    I->content = realloc(I->content, statbuf.st_size*sizeof(HEDByte));

    strncpy(I->file_name, filename, 128);

    I->content_length = statbuf.st_size;

    uint8_t c; // Single c read
    for(int i = 0; i < statbuf.st_size; i++){
        fread(&c, 1, 1, fp);
        I->content[i].o.value = c;
        I->content[i].c.value = c;
        I->content[i].is_original = true;
        I->content[i].g = -1;
    }

    // Check if the file is readonly, and warn the user about that.
    if (access(filename, W_OK) == -1) {
        editor_set_status(STATUS_WARNING, "\"%s\" (%d bytes) [readonly]",
            I->file_name, I->content_length);
        I->read_only = true;
    } else {
        editor_set_status(STATUS_INFO, "\"%s\" (%d bytes)",
            I->file_name, I->content_length);
        I->read_only = false;
    }

    fclose(fp);
}

// Opens the file and stores the content
void editor_write_file(char* name){
    FILE *fp;
    // If we have argument try to save to that file
    if (name != NULL && strlen(name) != 0){
        fp = fopen(name, "wb");
        if (fp == NULL){
            editor_set_status(STATUS_ERROR,
                "Unable to open '%s' for writing: %s",
                name, strerror(errno));
            return;
        }
        strncpy(I->file_name, name, 128);
    } else {
        // If no argument try to save to the previous opened file, if any
        if (strlen(I->file_name) == 0) {
            editor_set_status(STATUS_ERROR, "No file name");
            return;
        } else {
            fp = fopen(I->file_name, "wb");
            if (fp == NULL){
                editor_set_status(STATUS_ERROR,
                    "Unable to open '%s' for writing: %s",
                    I->file_name, strerror(errno));
                return;
            }
        }
    }



    int i;
    for(i = 0; i < I->content_length; i++){
        fwrite(&I->content[i].c, 1, 1, fp);
    }

    editor_set_status(STATUS_INFO, "written %d bytes", i);

    I->dirty = false;

    fclose(fp);
}

void editor_calculate_bytes_per_line(){

    // Calculate the maximum hex+ascii bytes that fits in the screen size
    // one byte = 2 chars
    int bytes_per_line = g_config->bytes_group; // Atleast write one byte group
    int filled;

    while((bytes_per_line / g_config->bytes_group) < g_config->groups_per_line)
    {
        bytes_per_line += g_config->bytes_group;
        filled = 10 + bytes_per_line * 2
            + (bytes_per_line/g_config->bytes_group) * PADDING + bytes_per_line;
        if (filled >= I->screen_cols){
            bytes_per_line -= g_config->bytes_group;
            break;
        }

    }

    I->bytes_per_line = bytes_per_line;
}

void editor_command_set_run(HEDBuff* cmd) {
    HEDBuff* key = buff_create();
    HEDBuff* value = buff_create();

    if (cmd->len > 0){
        config_parse_line_key_value(cmd, key, value);
        if (key->len > 0){
            int ret = config_update_key_value(g_config, key, value);
            if (ret == 0) {
                editor_set_status(STATUS_INFO, "Set '%s', to '%s'",
                    key->content, value->content);
            } else if (ret == 2){
                editor_set_status(STATUS_ERROR, "Invalid value for option \"%s\"",
                    key->content);
            } else {
                editor_set_status(STATUS_ERROR, "Invalid option \"%s\"",
                    key->content);
            }
        }
    }

    editor_calculate_bytes_per_line();

    buff_remove(key);
    buff_remove(value);

}

int editor_close_file() {
    if(I->dirty){
        editor_set_status(STATUS_ERROR, "No write since last change");
        return 0;
    }

    I->file_name[0] = 0;

    I->content_length = 0;

    return 1;
}


void editor_render_ascii(int row, unsigned int start, unsigned int len){

    HEDByte *c;
    HEDBuff* buff = I->buff;
    int offset = start;

    for(int i = 0; i < len; i++){
        // Get byte to write
        offset = start + i;
        if(I->content_length > 0) {
            c = &I->content[offset];
        }

        // Found match
        if(offset >= I->last_found.start && offset <= I->last_found.end){
            term_set_format_buff(buff, BG_BLUE);
        }

        // Selection
        if(offset >= I->selection.start && offset <= I->selection.end){
            term_set_format_buff(buff, BG_LIGHT_GRAY);
        }

        // Cursor
        if(I->cursor_y == row){
            if(I->cursor_x == i){
                if(I->in_ascii){
                    term_set_format_buff(buff, BG_YELLOW);
                    term_set_format_buff(buff, FG_BLACK);
                }else{
                    term_set_format_buff(buff, FORMAT_INVERTED);
                }
            }
        }

        if(I->content_length > 0) {
            if(c->c.value != c->o.value
                    || !c->is_original){
                term_set_format_buff(buff, FG_RED);
            }

            if(isprint(c->c.value)){
                buff_vappendf(buff, "%c", c->c.value);
            }else{
                buff_append(buff, ".", 1);
            }
        }else{
            buff_append(buff, ".", 1);
        }


        term_set_format_buff(buff, FORMAT_RESET);
    }

    // Fill the ascii
    while(len < I->bytes_per_line){
        buff_append(buff, " ", 1);
        len++;
    }

    if(I->in_ascii == true){
        term_set_format_buff(buff, FG_YELLOW);
        buff_append(buff, "|", 1);
        term_set_format_buff(buff, FORMAT_RESET);
    }else{
        buff_append(buff, " ", 1);
    }
}


unsigned int editor_offset_at_cursor(){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    unsigned int offset = (I->cursor_y - 1 + I->scrolled) *
                          (I->bytes_per_line) + (I->cursor_x);
    if (offset <= 0) {
        return 0;
    }
    if (offset >= I->content_length) {
        return I->content_length - 1;
    }
    return offset;
}

void editor_cursor_offset(unsigned int offset){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    I->cursor_x = offset % I->bytes_per_line;
    I->cursor_y = offset / I->bytes_per_line - I->scrolled + 1;
}

void editor_check_scroll_top_limit(){

    // Max scroll
    unsigned int top_limit = I->content_length/I->bytes_per_line
                             - (I->screen_rows-3);
    if(I->scrolled >= top_limit){
        I->scrolled = top_limit;
    }
}

void editor_cursor_offset_scroll(int offset){
    if(offset > I->content_length){
        // Out of bounds
        return;
    }

    // Check if offset is in view range
    unsigned int offset_min = I->scrolled * I->bytes_per_line;
	unsigned int offset_max = offset_min + ((I->screen_rows-1)
                              * I->bytes_per_line);

    if (offset >= offset_min && offset <= offset_max) {
        editor_cursor_offset(offset);
        return;
    }

    I->scrolled = (int) (offset / I->bytes_per_line - ((I->screen_rows-2)/2));

    if (I->scrolled <= 0) {
        I->scrolled = 0;
    }

    editor_check_scroll_top_limit();

    editor_cursor_offset(offset);
}


void editor_scroll(int units) {

    I->scrolled += units;

    editor_check_scroll_top_limit();

}

void editor_move_cursor(int dir, int amount){

    // Don't move if file empty
    if(I->content_length == 0){
        return;
    }

    for(int i = 0; i < amount; i++){
        switch (dir){
            case KEY_UP: I->cursor_y--; break;
            case KEY_DOWN: I->cursor_y++; break;
            case KEY_LEFT: I->cursor_x--; break;
            case KEY_RIGHT: I->cursor_x++; break;
        }

        // Beggining of file, stop
        if(I->cursor_x <= 0 && I->cursor_y <= 1 && I->scrolled <= 0){
            I->cursor_x = 0;
            I->cursor_y = 1;
            return;
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
            I->cursor_y++;
            I->cursor_x = 0;
        }

        // Stop top file limit
        if(I->cursor_y <= 1 && I->scrolled <= 0){
            I->cursor_y = 1;
        }

        if(I->cursor_y > I->screen_rows - 2){
            I->cursor_y = I->screen_rows - 2;
            editor_scroll(1);
        }else if (I->cursor_y < 1 && I->scrolled > 0){
            I->cursor_y = 1;
            editor_scroll(-1);
        }

        unsigned int offset = editor_offset_at_cursor();

        if (offset >= I->content_length - 1) {
            editor_cursor_offset(offset);
        }
    }

}

void editor_update_visual_selection(){

    unsigned int offset = editor_offset_at_cursor();

    // If we moving backwards, the start of selection
    if(offset <= I->selection.start && !I->selection.is_backwards){
        I->selection.end          = I->selection.start;
        I->selection.is_backwards = true;
    }else if(offset >= I->selection.end && I->selection.is_backwards){
    // If we are crossing forwards again
        I->selection.start        = I->selection.end;
        I->selection.is_backwards = false;
    }

    if(I->selection.is_backwards){
        I->selection.start = offset;
    }else{
        I->selection.end = offset;
    }
}

void editor_move_cursor_visual(int dir, int amount){

    editor_move_cursor(dir, amount);

    editor_update_visual_selection();

}

void editor_define_grammar_offset(int start, int end){
    int index = grammar_add(I->grammars, "", start, end, COLOR_GREEN_BG);
    for(int i = start; i <= end; i++){
        I->content[i].g = index;
    }
}

void editor_define_grammar_cursor(){
    unsigned int offset = editor_offset_at_cursor();
    editor_define_grammar_offset(offset, offset);
}

void editor_render_content(HEDBuff* buff){

    unsigned int line_chars = 0;
    unsigned int line_bytes = 0;
    unsigned int chars_until_ascii = 0;

    unsigned int line_offset_start = 0;

    int row = 0;

    int offset;

    chars_until_ascii = 10 + I->bytes_per_line * 2 + I->bytes_per_line/2;

    int offset_start = I->scrolled * I->bytes_per_line;

    if(offset_start >= I->content_length){
        offset_start = I->content_length;
    }

    // ruler + status
    int bytes_per_screen = I->bytes_per_line * (I->screen_rows - 2);
    int offset_end = bytes_per_screen + offset_start;

    // Don't show more than content_length
    if(offset_end > I->content_length ){
        offset_end = I->content_length;
    }

    // Empty file
    if(offset_end == 0){
        offset_end ++;
    }

    for (offset = offset_start; offset < offset_end; offset++){
        // New row
        if(offset % I->bytes_per_line == 0 ){
            line_chars = 0;
            line_bytes = 0;
            line_offset_start = offset;
            term_set_format_buff(buff, FG_BLUE);
            buff_vappendf(buff, "%08x: ", offset);
            line_chars += 10;
            row++;
            // Cursor side
            if(I->in_ascii == false){
                // Color yellow
                term_set_format_buff(buff, FG_YELLOW);
                buff_append(buff, "|", 1);
            }else{
                buff_append(buff, " ", 1);
            }
            line_chars += 1;
            // End Cursor side
        }

        term_set_format_buff(buff, FORMAT_RESET);

        // Found match
        if(offset >= I->last_found.start && offset <= I->last_found.end){
            term_set_format_buff(buff, BG_BLUE);
        }

        // Selection
        if(offset >= I->selection.start && offset <= I->selection.end){
            term_set_format_buff(buff, BG_LIGHT_GRAY);
        }

        // Cursor
        if(I->cursor_y == row){
            if(I->cursor_x == line_bytes){
                if(I->in_ascii){
                    term_set_format_buff(buff, FORMAT_INVERTED);
                }else{
                    term_set_format_buff(buff, BG_YELLOW);
                    term_set_format_buff(buff, FG_BLACK);
                }
            }
        }

        if(I->content_length == 0){
            buff_vappendf(buff, "%02x", 0);
        }else{

            // Grammar color
            enum color_bg color = grammar_color_id(I->grammars,
                I->content[offset].g);
            if(color != COLOR_NOCOLOR){
                term_set_format_buff(buff, color);
            }

            // Write the value on the screen (HEDBuff)
            // If the value is changed
            if(I->content[offset].c.value != I->content[offset].o.value
                || !I->content[offset].is_original){
                term_set_format_buff(buff, FG_RED);
            }

            buff_vappendf(buff, "%02x",
                (unsigned int) I->content[offset].c.value);

        }

        line_chars += 2;
        line_bytes += 1;
        // Reset color
        term_set_format_buff(buff, FORMAT_RESET);

        // Every group, write a separator of len PADDING, unless its the
        // last in line or the last row
        if((line_bytes % g_config->bytes_group == 0) &&
            (line_bytes != I->bytes_per_line)){
            for(int s=0; s < PADDING; s++){
                buff_append(buff, " ", 1);
                line_chars += 1;
            }
        }
        // If end of line, write ascii and new line
        if((offset + 1) % I->bytes_per_line == 0){
            // Cursor side
            term_set_format_buff(buff, FG_YELLOW);
            if(I->in_ascii == false){
                buff_append(buff, "| ", 2);
            }else{
                buff_append(buff, " |", 2);
            }
            line_chars += 2;
            // End Cursor side
            term_set_format_buff(buff, FORMAT_RESET);
            editor_render_ascii(row, line_offset_start, I->bytes_per_line);
            line_chars += I->bytes_per_line; // ascii chars
            term_clear_line_end_buff(buff);
            buff_append(buff, "\r\n", 2);
        }
    }

    // If we need padding for ascii
    if(offset % I->bytes_per_line != 0){
        // Fill until the ascii chars in case of short line
        while( line_chars < chars_until_ascii){
                buff_append(buff, " ", 1);
                line_chars++;
        }
        term_set_format_buff(buff, FG_YELLOW);
        if(I->in_ascii == false){
            buff_append(buff, "| ", 2);
        }else{
            buff_append(buff, " |", 2);
        }
        term_set_format_buff(buff, FORMAT_RESET);
        // Render ascii
        editor_render_ascii(row, line_offset_start, line_bytes);
        term_clear_line_end_buff(buff);
    }


    while(++row <= I->screen_rows - 2){
        // clear everything up until the status bar and ruler
        term_goto_buff(buff, row, 0);
        term_clear_line_end_buff(buff);
    }


}

void editor_render_command(char *command){
    char buffer[32];
    term_goto(I->screen_rows-EDITOR_COMMAND_STATUS_OFFSET_Y,
        I->screen_cols-EDITOR_COMMAND_STATUS_OFFSET_X);
    int w = sprintf(buffer, "%s", command);
    term_print(buffer, w);
}

// Modes

void editor_start_mode_normal(){
    // Reset selection
    I->selection.start = -1;
    I->selection.end = -1;
}

void editor_start_mode_replace(){
    if (I->read_only && !I->warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        I->warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(I->repeat_buff->len != 0){
        I->repeat_buff->len = 0;
    }
}

void editor_start_mode_insert(){
    if (I->read_only && !I->warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        I->warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(I->repeat_buff->len != 0){
        I->repeat_buff->len = 0;
    }
}

void editor_start_mode_visual(){
    unsigned int offset = editor_offset_at_cursor();
    I->selection.start = offset;
    I->selection.end   = offset;
}

void editor_start_mode_cursor(){
}

void editor_start_mode_command(){
}

void editor_start_mode_append() {
    if (I->read_only && !I->warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        I->warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(I->repeat_buff->len != 0){
        I->repeat_buff->len = 0;
    }
}

void editor_set_mode(enum editor_mode mode){
    editor_set_status(STATUS_INFO, "");
    I->mode = mode;
    switch(I->mode){
        case MODE_NORMAL:   editor_start_mode_normal(); break;
        case MODE_INSERT:   editor_start_mode_insert(); break;
        case MODE_APPEND:   editor_start_mode_append(); break;
        case MODE_REPLACE:  editor_start_mode_replace(); break;
        case MODE_CURSOR:   editor_start_mode_cursor(); break;
        case MODE_VISUAL:   editor_start_mode_visual(); break;
        case MODE_COMMAND:  editor_start_mode_command(); break;
        case MODE_GRAMMAR:  break;
    }
}

void editor_define_grammar_visual(){
    editor_define_grammar_offset(I->selection.start, I->selection.end);
    editor_set_mode(MODE_NORMAL);
}

void editor_render_ruler_left(HEDBuff* buff){
    // Left ruler
    // Start writting the text now
    term_goto_buff(buff, I->screen_rows - EDITOR_RULER_OFFSET_Y, 0);

    // Write the left ruler
    if(strlen(I->file_name) != 0){
        buff_vappendf(buff, "%s ", I->file_name);
    }else{
        buff_vappendf(buff, "[No Name]");
    }

    if(I->dirty){
        buff_append(buff, "[+]", 3);
    }
    if(I->read_only){
        buff_append(buff, "[RO]", 4);
    }
}

void editor_render_ruler_right(HEDBuff* buff){
    unsigned int offset = editor_offset_at_cursor();
    // If no data percentage will be 100%
    unsigned int percentage = 100;

    // Right ruler position
    term_goto_buff(buff, I->screen_rows-1,
                          I->screen_cols - EDITOR_RULER_RIGHT_OFFSET_X);

    if(I->content_length > 1){
        percentage = offset*100/(I->content_length-1);
    }

    // Write the right ruler
    buff_vappendf(buff, "0x%08x (%d)  %d%%", offset, offset, percentage);
}

void editor_render_ruler(HEDBuff* buff) {

    // Move to position and clear line
    term_goto_buff(buff, I->screen_rows - EDITOR_RULER_OFFSET_Y, 0);
    term_clear_line_buff(buff);

    // Write the line background and foreground colors
    term_set_format_buff(buff, BG_LIGHT_GRAY);
    term_set_format_buff(buff, FG_BLACK);

    // Create an empty line colored with the background
    for(int i = 0; i < I->screen_cols; i++){
        buff_append(buff, " ", 1);
    }

    editor_render_ruler_left(buff);
    editor_render_ruler_right(buff);

    term_clear_line_end_buff(buff);
    term_set_format_buff(buff, FORMAT_RESET);
}

void editor_render_status_mode(HEDBuff* buff) {

    term_set_format_buff(buff, FG_GREEN);

    switch(I->mode){
        case MODE_NORMAL:   buff_vappendf(buff,""); break;
        case MODE_INSERT:   buff_vappendf(buff,"-- INSERT --  "); break;
        case MODE_APPEND:   buff_vappendf(buff,"-- APPEND --  "); break;
        case MODE_REPLACE:  buff_vappendf(buff,"-- REPLACE --  "); break;
        case MODE_CURSOR:   buff_vappendf(buff,"-- CURSOR --  "); break;
        case MODE_VISUAL:   buff_vappendf(buff,"-- VISUAL --  "); break;
        case MODE_COMMAND:  buff_vappendf(buff,":"); break;
        case MODE_GRAMMAR:  break;
    }

    term_set_format_buff(buff, FORMAT_RESET);

}

void editor_render_status(HEDBuff* buff){

    // Move to (0,screen_rows);
    term_goto_buff(buff, I->screen_rows, 0);

    editor_render_status_mode(buff);

    int len = I->status_message->len;
    if (I->screen_cols <= len) {
        len = I->screen_cols;
        buff_append(buff, I->status_message->content, len-3);
        buff_append(buff, "...", 3);
    }
    buff_append(buff, I->status_message->content, len);
    term_clear_line_end_buff(buff);
}

void editor_refresh_screen(){

    HEDBuff* buff = I->buff;

    // Clear the content of the buffer
    // To speed the screen refresh, we use buff_clear_dirty, instead
    // of buff_clear
    buff_clear_dirty(buff);

    term_cursor_hide();
    // Move cursor top left
    term_goto_buff(buff, 1, 1);

    editor_render_content(buff);
    editor_render_status(buff);
    editor_render_ruler(buff);

    // Show msg info
    if (!I->content_length && !I->init_msg) {
        term_goto_buff(buff, I->screen_rows/2 - 4, I->screen_cols/2 - 5);
        buff_vappendf(buff, "HED - hex editor");
        term_goto_buff(buff, I->screen_rows/2 - 2, I->screen_cols/2 - 3);
        buff_vappendf(buff, "version %s", HED_VERSION_SHORT);
        term_goto_buff(buff, I->screen_rows/2, I->screen_cols/2 - 5);
        buff_vappendf(buff, "by Ferran Celades");
        fflush(stdout);
    }

    // Move to the last line of the screen
    term_goto_buff(buff, I->screen_rows, 1);

    // Write the buffer on the screen
    term_print(buff->content, buff->len);
}

void editor_write_byte_offset(unsigned char new_byte, unsigned int offset){

    I->content[offset].c.value = new_byte;
}

void editor_write_cursor(unsigned char new_byte){

    unsigned int offset = editor_offset_at_cursor();
    editor_write_byte_offset(new_byte, offset);
    editor_move_cursor(KEY_RIGHT, 1);
}

void editor_prepare_write_repeat(char ch){

    if((I->repeat_buff->len+1 >= I->repeat_buff->capacity)){
        I->repeat_buff->content = realloc(I->repeat_buff->content, I->repeat_buff->capacity*2);
        I->repeat_buff->capacity *= 2;
    }
    I->repeat_buff->content[I->repeat_buff->len] = ch;
    I->repeat_buff->len++;
}

void editor_replace_offset(unsigned int offset, unsigned char c){

    HEDByte old_byte;

    if (!I->content_length) {
        editor_set_status(STATUS_WARNING, "File is empty, nothing to replace");
        return;
    }

    if(offset >= I->content_length){
        return;
    }

    I->dirty = true;

    if(I->in_ascii){
        // Create the action
        // Original, Current, grammar
        HEDByte action_byte = {{I->content[offset].c.value},
                               {c},
                               I->content[offset].is_original,
                               I->content[offset].g};
        action_add(I->action_list, ACTION_REPLACE, offset, action_byte,
            I->repeat);
        editor_write_byte_offset(c, offset);
        editor_move_cursor(KEY_RIGHT, 1);
        return;
    }

    // Second octet
    if(I->last_write_offset == offset){
        old_byte = I->content[offset];
        // One octet already written in this position

        if (g_config->replace_nibble == 0) {
            I->content[offset].c.nibble.top =
                    I->content[offset].c.nibble.bottom;
            I->content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            I->content[offset].c.nibble.bottom = utils_hex2int(c);
        }

        if (old_byte.c.value != I->content[offset].c.value) {
            I->content[offset].is_original = false;
        }
        // Modify the last action to reflect the 2nd nibble
        I->action_list->last->b.c.value = I->content[offset].c.value;

        editor_move_cursor(KEY_RIGHT, 1);

        I->last_write_offset = -1;
    }else{

        old_byte = I->content[offset];

        if (g_config->replace_nibble == 0) {
            I->content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            I->content[offset].c.nibble.top = utils_hex2int(c);
        }

        if (old_byte.c.value != I->content[offset].c.value) {
            I->content[offset].is_original = false;
        }

        I->last_write_offset = offset;

        // Create the action
        HEDByte action_byte = {{old_byte.c.value},
                                {I->content[offset].c.value},
                                old_byte.is_original,
                                I->content[offset].g};
        action_add(I->action_list, ACTION_REPLACE, offset, action_byte, I->repeat);
    }

}

void editor_replace_cursor(char c){

    unsigned int offset = editor_offset_at_cursor();
    editor_replace_offset(offset, c);

}

void editor_replace_repeat_last(int times){
    I->repeat = I->repeat_buff->len * times;
    if (!I->in_ascii) {
        I->repeat /= 2;
    }
    for(int c=0; c < I->repeat_buff->len; c++){
        editor_replace_cursor(I->repeat_buff->content[c]);
    }

}


void editor_replace_cursor_repeat(){

    for(int r=0; r < I->repeat - 1; r++){
        for(int c=0; c < I->repeat_buff->len; c++){
            editor_replace_cursor(I->repeat_buff->content[c]);
        }
    }
}

void editor_replace_visual(){

    int c = read_key();

    // action repeat
    I->repeat = I->selection.end - I->selection.start + 1;

    // No need to scroll
    editor_cursor_offset(I->selection.start);

    if(isxdigit(c)){
        if(I->in_ascii){
            for(int i= I->selection.start; i <= I->selection.end; i++){
                editor_replace_offset(i, c);
            }
        }else{
            int c2 = read_key();
            if(isxdigit(c2)){
                for(int i= I->selection.start; i <= I->selection.end; i++){
                    editor_replace_offset(i, c); // First octet
                    editor_replace_offset(i, c2); // Second octet
                }

            }else{
                for(int i= I->selection.start; i <= I->selection.end; i++){
                    editor_replace_offset(i, c); // First octet
                    editor_replace_offset(i, c); // Second octet
                }

            }
        }
    }

    editor_set_mode(MODE_NORMAL);

}

void editor_insert_offset(unsigned int offset, unsigned char c, bool append){

    char new_byte = 0;

    if(offset >= I->content_length && I->content_length != 0){
        return;
    }

    I->dirty = true;
    I->init_msg = true;

    if(I->in_ascii){
        I->content = realloc(I->content, (I->content_length + 1)*sizeof(HEDByte));
        memmove(&I->content[offset+1], &I->content[offset],
            (I->content_length - offset)*sizeof(HEDByte));
        I->content_length++;

        // Make sure to set the original to 0 on insert
        I->content[offset].o.value = 0;
        I->content[offset].is_original = false;
        // Less significative first
        I->content[offset].c.value = c;

        HEDByte action_byte = {{0},{c},false,0};

        // Create the action
        action_add(I->action_list, ACTION_INSERT, offset, action_byte, I->repeat);
        editor_move_cursor(KEY_RIGHT, 1);
        return;
    }

    // Second octet
    if(I->last_write_offset == offset){

        if (g_config->insert_nibble == 0) {
            I->content[offset].c.nibble.top =
                        I->content[offset].c.nibble.bottom;
            I->content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            I->content[offset].c.nibble.bottom = utils_hex2int(c);
        }

        // Modify the last action to reflect the 2nd nibble
        I->action_list->last->b.c.value = I->content[offset].c.value;

        if (append == false) {
            editor_move_cursor(KEY_RIGHT, 1);
        }

        I->last_write_offset = -1;

    }else{
        // If we append, add offset 1, unless the file is empty
        if (append && I->content_length > 0) {
            offset++;
        }
        // First octet
        // Increase allocation by one byte_t
        // TODO: only if not space already
        I->content = realloc(I->content, (I->content_length + 1)
                             * sizeof(HEDByte));
        memmove(&I->content[offset+1], &I->content[offset],
                (I->content_length - offset)*sizeof(HEDByte));
        I->content_length++;

        new_byte = utils_hex2int(c);

        // Make sure to set the original to 0 and on insert
        I->content[offset].o.value = 0;
        I->content[offset].c.value = 0;
        I->content[offset].is_original = false;

        if (g_config->insert_nibble == 0) {
            // Less significative first
            I->content[offset].c.nibble.bottom = new_byte;
        } else {
            I->content[offset].c.nibble.top = new_byte;
        }

        I->last_write_offset = offset;

        HEDByte action_byte = {{0}, {new_byte}, false, 0};

        if (append && I->content_length > 0) {
            editor_move_cursor(KEY_RIGHT, 1);
        }

        if (append) {
            action_add(I->action_list, ACTION_APPEND, offset, action_byte, I->repeat);
        } else {
            action_add(I->action_list, ACTION_INSERT, offset, action_byte, I->repeat);
        }

    }

}

void editor_append_cursor(char c) {
    unsigned int offset = editor_offset_at_cursor();
    editor_insert_offset(offset, c, true);
}

void editor_insert_cursor(char c) {
    unsigned int offset = editor_offset_at_cursor();
    editor_insert_offset(offset, c, false);

}

void editor_append_repeat_last(int times) {
    I->repeat = I->repeat_buff->len * times;
    if (!I->in_ascii) {
        I->repeat /= 2;
    }
    for(int c=0; c < I->repeat_buff->len; c++){
        editor_append_cursor(I->repeat_buff->content[c]);
    }
}

void editor_append_cursor_repeat() {
    for(int r=0; r < I->repeat - 1; r++){
        for(int c=0; c < I->repeat_buff->len; c++){
            editor_append_cursor(I->repeat_buff->content[c]);
        }
    }
}

void editor_insert_repeat_last(int times) {
    I->repeat = I->repeat_buff->len * times;
    if (!I->in_ascii) {
        I->repeat /= 2;
    }
    for(int c=0; c < I->repeat_buff->len; c++){
        editor_insert_cursor(I->repeat_buff->content[c]);
    }

}

void editor_insert_cursor_repeat() {
    for(int r=0; r < I->repeat - 1; r++){
        for(int c=0; c < I->repeat_buff->len; c++){
            editor_insert_cursor(I->repeat_buff->content[c]);
        }
    }
}

void editor_undo_insert_offset(unsigned int offset){
    memmove(&I->content[offset], &I->content[offset+1],
        (I->content_length - offset-1)*sizeof(HEDByte));
    I->content = realloc(I->content, (I->content_length - 1) * sizeof(HEDByte));
    I->content_length--;
    I->dirty = true;
}

void editor_redo_insert_offset(unsigned int offset, HEDByte b){
    I->content = realloc(I->content, (I->content_length + 1) * sizeof(HEDByte));
    memmove(&I->content[offset+1], &I->content[offset],
        (I->content_length - offset)*sizeof(HEDByte));
    I->content[offset].c = b.c;
    I->content[offset].o = b.o;
    I->content[offset].is_original = b.is_original;
    I->content_length++;
    I->dirty = true;
}

void editor_undo_delete_offset(unsigned int offset, HEDByte b){
    editor_redo_insert_offset(offset, b);
    I->dirty = true;
}

void editor_delete_offset(unsigned int offset) {
    if(I->content_length > 0){
        HEDByte old_byte = I->content[offset];
        memmove(&I->content[offset], &I->content[offset+1],
            (I->content_length - offset-1)*sizeof(HEDByte));
        I->content = realloc(I->content, (I->content_length - 1)
            * sizeof(HEDByte));
        HEDByte action_byte = {{old_byte.c.value},
                                {old_byte.o.value},
                                old_byte.is_original ,
                                old_byte.g};
        action_add(I->action_list, ACTION_DELETE, offset, action_byte, I->repeat);
        I->content_length--;
    } else {
        editor_set_status(STATUS_WARNING, "File is empty");
    }
    I->dirty = true;
}

void editor_delete_cursor(){
    unsigned int offset = editor_offset_at_cursor();
    editor_delete_offset(offset);
    if(offset > 0 && offset == I->content_length){
        editor_move_cursor(KEY_LEFT, 1);
    }else{
        editor_cursor_offset(offset);
    }
    I->dirty = true;
}

void editor_delete_cursor_repeat(){
    for(int r=0; r < I->repeat; r++){
        editor_delete_cursor();
    }
}

void editor_delete_visual(){
    I->repeat = I->selection.end - I->selection.start + 1;
    for(int i= I->selection.start; i <= I->selection.end; i++){
        editor_delete_offset(I->selection.start);
    }
    editor_cursor_offset(I->selection.start);
    editor_set_mode(MODE_NORMAL);
    I->dirty = true;
}


void editor_reset_write_repeat(){
    I->last_write_offset = -1;
}

void editor_redo_delete_offset(unsigned int offset, HEDByte b){
    memmove(&I->content[offset], &I->content[offset+1],
        (I->content_length - offset-1)*sizeof(HEDByte));
    I->content = realloc(I->content, (I->content_length - 1)*sizeof(HEDByte));
    I->content_length--;
    editor_cursor_offset(offset);
    I->dirty = true;
}

void editor_undo_redo_replace_offset(unsigned int offset, HEDByte b){
    HEActionList *list = I->action_list;
    // Swap original and current in action
    list->current->b.o = I->content[offset].c;
    list->current->b.c = I->content[offset].o;
    list->current->b.is_original = I->content[offset].is_original;

    I->content[offset].c = b.o;
    I->content[offset].is_original = b.is_original;
    I->dirty = true;
}


void editor_repeat_last_action(){

    HEActionList *list = I->action_list;
    int times = I->repeat;
    for(int i = 0; i < times; i++) {
        switch(list->last->type){
            case ACTION_BASE: break;
            case ACTION_REPLACE: editor_replace_repeat_last(times); break;
            case ACTION_INSERT: editor_insert_repeat_last(times); break;
            case ACTION_APPEND: editor_append_repeat_last(times); break;
            case ACTION_DELETE: break;

        }
    }

}

void editor_redo(){

    HEActionList *list = I->action_list;

    // If newest change
    if(list->current->next == NULL){
        editor_set_status(STATUS_INFO, "Already at newest change");
        return;
    }

    for(int i = 0; i < I->repeat; i++){

        // Undo multiple actions
        // We start from action base. To redo the last change we have to go to
        // the next action in the list
        //list->current = list->current->next;
        int repeat = list->current->next->repeat;

        for(int r = 0; r < repeat; r++) {

            list->current = list->current->next;
            unsigned int offset = list->current->offset;
            HEDByte b = list->current->b;

            switch(list->current->type){
                case ACTION_BASE:
                    editor_set_status(STATUS_INFO, "Already at newest change");
                    return;
                case ACTION_REPLACE:
                    // Store modified value in case of undo
                    editor_undo_redo_replace_offset(offset, b);
                    editor_cursor_offset_scroll(offset);
                    break;
                case ACTION_INSERT:
                    editor_redo_insert_offset(offset, b);
                    editor_cursor_offset_scroll(offset);
                    break;
                case ACTION_DELETE:
                    editor_redo_delete_offset(offset, b); break;
                case ACTION_APPEND:
                    editor_redo_insert_offset(offset, b);
                    editor_cursor_offset_scroll(offset + 1);
                    break;
            }
            // if (list->current->next != NULL){
            //     list->current = list->current->next;
            // }
        }
        I->dirty = true;
    }

    //action_list_print(list);

}

void editor_undo(){

    HEActionList *list = I->action_list;

    if (list->current->type == ACTION_BASE) {
        editor_set_status(STATUS_INFO, "Already at oldest change");
        I->dirty = false;
        return;
    }

    for(int i = 0; i < I->repeat; i++){

        // Undo multiple actions
        int repeat = list->current->repeat;

        for(int r = 0; r < repeat; r++) {

            unsigned int offset = list->current->offset;
            HEDByte b = list->current->b;

            switch(list->current->type){
                case ACTION_BASE:
                    editor_set_status(STATUS_INFO, "Already at oldest change");
                    I->dirty = false;
                    return;
                case ACTION_REPLACE:
                    // Store modified value in case of redo
                    editor_undo_redo_replace_offset(offset, b);
                    editor_cursor_offset_scroll(offset);
                    break;
                case ACTION_INSERT:
                    editor_undo_insert_offset(offset);
                    editor_cursor_offset_scroll(offset);
                    break;
                case ACTION_DELETE:
                    editor_undo_delete_offset(offset, b);
                    editor_cursor_offset_scroll(offset);
                    break;
                case ACTION_APPEND:
                    editor_undo_insert_offset(offset);
                    editor_cursor_offset_scroll(offset - 1);
                    break;

            }

            list->current = list->current->prev;
        }
        I->dirty = true;
    }

    //action_list_print(list);
}

void editor_toggle_cursor(){
    I->in_ascii = (I->in_ascii == true) ? false: true;
}


void editor_process_quit(bool force){

    if(force){
        exit(0);
    }

    if(I->dirty){
        editor_set_status(STATUS_ERROR, "No write since last change");
        return;
    }

    exit(0);
}

void editor_process_command(){

    char* command = I->read_buff->content;
    int len = I->read_buff->len;

    if(len == 0){
        return;
    }

    // Go offset
    if (isdigit(command[0])) {
        int offset = strtol(command, NULL, 0);
        editor_cursor_offset_scroll(offset);
        return;
    }

    switch(command[0]){
        case 'w':
            if(len == 1){
                editor_write_file(NULL);
            }else if(len == 2 && command[1] == 'q'){
                editor_write_file(NULL);
                editor_process_quit(false);
            }else if(len == 3 && command[1] == 'q' && command[2] == '!'){
                editor_write_file(NULL);
                editor_process_quit(true);
            }
            if (len > 1){
                if (command[1] == ' '){
                    editor_write_file(&command[2]);
                    return;
                }
                if (strcmp(command, "write ")){
                    editor_write_file(&command[6]);
                    return;
                }
            }
            break;
        case 'q':
            if(len == 1){
                editor_process_quit(false);
            }else if(len == 2 && command[1] == '!'){
                editor_process_quit(true);
            }
            break;
        case 'e':
            if(len > 1) {
                if (command[1] == ' '){
                    if(editor_close_file()){
                        editor_open_file(&command[2]);
                        return;
                    }
                }
                if (strcmp(command, "edit ")){
                    if(editor_close_file()){
                        editor_open_file(&command[5]);
                        return;
                    }
                }
            }
            break;
        case 's':
            if(len > 1) {
                if(strcmp(command, "set ")) {
                    editor_command_set_run(I->read_buff);
                    return;
                }
            }
            break;
        case 'o':
            if(len > 2) {
                if (command[1] == ' '){
                    long offset = strtol(&command[2], NULL, 0);
                    editor_cursor_offset_scroll(offset);
                    return;
                }
                if (strcmp(command, "offset ")){
                    long offset = strtol(&command[7], NULL, 0);
                    editor_cursor_offset_scroll(offset);
                    return;
                }

            }
            break;
        default:
        editor_set_status(STATUS_ERROR, "Not an editor command: %s", command);
    }

    buff_clear(I->read_buff);

}


void editor_read_string(char *prompt){
    read_line(I->read_buff, prompt);
}

void editor_read_command(){
    editor_read_string(":");
}

void editor_process_search_repeat(SEARCH_DIRECTION direction, bool is_repeat) {

    HEDBuff* search_pattern;

    if (I->in_ascii) {
        search_pattern = I->search_buff;
    } else {
        if (!utils_hexonly(I->search_buff)) {
            editor_set_status(STATUS_WARNING, "Search pattern is not hex");
            return;
        }
        search_pattern = buff_create();
        utils_hexstring_to_buff(I->search_buff, search_pattern);
    }

    int offset = editor_offset_at_cursor();

    if (is_repeat) {
        if (direction == SEARCH_FORWARD) {
            offset++;
        } else {
            offset--;
        }
    }

    int found_offset = search_buffer(I->content, I->content_length,
        search_pattern, offset, direction);


    if (found_offset != SEARCH_NOT_FOUND) {
        editor_cursor_offset_scroll(found_offset);
        editor_set_status(STATUS_INFO, "Pattern found at 0x%x", found_offset);
        I->last_found.start = found_offset;
        if (I->in_ascii) {
            I->last_found.end = found_offset + I->search_buff->len - 1;
        } else {
            I->last_found.end = found_offset + search_pattern->len - 1;
        }
    } else {
        editor_set_status(STATUS_WARNING, "Pattern not found");
        I->last_found.start = -1;
        I->last_found.end = -1;
    }

    if (!I->in_ascii) {
        buff_remove(search_pattern);
    }
}

void editor_process_search(SEARCH_DIRECTION direction) {

    term_clear_line_end();

    if (direction == SEARCH_FORWARD) {
        if (I->in_ascii) {
            read_line(I->search_buff, "(asc)/");
        } else {
            read_line(I->search_buff, "(hex)/");
        }
    } else {
        if (I->in_ascii) {
            read_line(I->search_buff, "(asc)?");
        } else {
            read_line(I->search_buff, "(hex)?");
        }
    }

    editor_process_search_repeat(direction, false);
}

void editor_increment_byte_cursor(int repeat){
    if (I->content_length > 0) {
        int offset = editor_offset_at_cursor();
        HEDByte action_byte = I->content[offset];
        action_byte.o.value = action_byte.c.value;
        I->content[offset].c.value += repeat;
        I->repeat = 1;
        action_add(I->action_list, ACTION_REPLACE, offset, action_byte, I->repeat);
    }
}

void editor_decrement_byte_cursor(int repeat){
    if (I->content_length > 0) {
        int offset = editor_offset_at_cursor();
        HEDByte action_byte = I->content[offset];
        action_byte.o.value = action_byte.c.value;
        I->content[offset].c.value -= repeat;
        I->repeat = 1;
        action_add(I->action_list, ACTION_REPLACE, offset, action_byte, I->repeat);
    }
}


// Process the key pressed
void editor_process_keypress(){

    char command[MAX_COMMAND];
    command[0] = '\0';

    int c;

    if(I->mode == MODE_COMMAND){
        editor_read_command();
        editor_set_mode(MODE_NORMAL);
        editor_process_command();
        return;
    }

    // Read first command char
    c = read_key();

    if(I->mode == MODE_NORMAL){
        // TODO: Implement key repeat correctly
        if(c != '0'){
            unsigned int count = 0;
            while(isdigit(c) && count < 5){
                command[count] = c;
                command[count+1] = '\0';
                editor_render_command(command);
                c = read_key();
                count++;
            }

            // Store in case we change mode
            I->repeat = atoi(command);
            if(I->repeat <= 0 ){
                I->repeat = 1;
            }
        }

        switch (c){
            case KEY_TAB: editor_toggle_cursor(); break;

            case KEY_LEFT:
            case 'h': editor_move_cursor(KEY_LEFT, I->repeat); break;
            case KEY_DOWN:
            case 'j': editor_move_cursor(KEY_DOWN, I->repeat); break;
            case KEY_UP:
            case 'k': editor_move_cursor(KEY_UP, I->repeat); break;
            case KEY_RIGHT:
            case 'l': editor_move_cursor(KEY_RIGHT, I->repeat); break;
			
            case KEY_PAGEUP: editor_move_cursor(KEY_UP, I->screen_rows); break;
            case KEY_PAGEDOWN: editor_move_cursor(KEY_DOWN, I->screen_rows); break;

            case 'w': editor_move_cursor(KEY_RIGHT,
                I->repeat * g_config->bytes_group); break;
            case 'b': editor_move_cursor(KEY_LEFT,
                I->repeat * g_config->bytes_group); break;

            //case 'd': editor_delete_cursor_repeat(); break;

            case '[': editor_decrement_byte_cursor(I->repeat); break;
            case ']': editor_increment_byte_cursor(I->repeat); break;

            // Modes
            case KEY_ESC: editor_set_mode(MODE_NORMAL); break;
            case 'i': editor_set_mode(MODE_INSERT); break;
            case 'c': editor_set_mode(MODE_CURSOR); break;
            case 'r': editor_set_mode(MODE_REPLACE); break;
            case 'v': editor_set_mode(MODE_VISUAL); break;
            case 'a': editor_set_mode(MODE_APPEND); break;
            // Append end of line
            case 'A':
                editor_move_cursor(KEY_RIGHT,
                I->bytes_per_line-1 - I->cursor_x);
                editor_set_mode(MODE_APPEND); break;
            case ':': editor_set_mode(MODE_COMMAND); break;

            // Search
            case '/': editor_process_search(SEARCH_FORWARD); break;
            case '?': editor_process_search(SEARCH_BACKWARD); break;
            case 'n': editor_process_search_repeat(SEARCH_FORWARD, true); break;
            case 'N': editor_process_search_repeat(SEARCH_BACKWARD, true); break;

            // Remove
            case KEY_DEL:
            case 'x': editor_delete_cursor_repeat(); break;

            // TODO: Repeat last write command
            case '.': editor_repeat_last_action(); break;

            // Undo/Redo
            case 'u': editor_undo(); break;
            case  KEY_CTRL_R: editor_redo(); break;

            // EOF
            case 'G':
                if(I->repeat != 1){
                    editor_cursor_offset_scroll(I->repeat);
                }else{
                    editor_cursor_offset_scroll(I->content_length-1);
                }
                break;
            case 'g':
                editor_render_command("g");
                c = read_key();
                if(c == 'g'){
                    editor_cursor_offset_scroll(0);
                }
                break;
            // Start of line
            case '0': I->cursor_x = 0; break;
            // End of line
            case '$': editor_move_cursor(KEY_RIGHT,
                I->bytes_per_line-1 - I->cursor_x); break;
			case 'Q' :
                editor_process_quit(false); break;
        }

        return;
    }

    if(I->mode == MODE_VISUAL){
        // TODO: Implement key repeat correctly
        if(c != '0'){
            unsigned int count = 0;
            while(isdigit(c) && count < 5){
                command[count] = c;
                command[count+1] = '\0';
                editor_render_command(command);
                c = read_key();
                count++;
            }

            // Store in case we change mode
            I->repeat = atoi(command);
            if(I->repeat <= 0 ){
                I->repeat = 1;
            }
        }

        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            editor_set_mode(MODE_NORMAL);
        }else{
            switch(c){

                case KEY_LEFT:
                case 'h': editor_move_cursor_visual(KEY_LEFT, I->repeat); break;
                case KEY_DOWN:
                case 'j': editor_move_cursor_visual(KEY_DOWN, I->repeat); break;
                case KEY_UP:
                case 'k': editor_move_cursor_visual(KEY_UP, I->repeat); break;
                case KEY_RIGHT:
                case 'l': editor_move_cursor_visual(KEY_RIGHT, I->repeat); break;
			    
                case KEY_PAGEUP: editor_move_cursor(KEY_UP, I->screen_rows); break;
                case KEY_PAGEDOWN: editor_move_cursor(KEY_DOWN, I->screen_rows); break;

                case 'd': editor_delete_visual(); break;

                case 'w': editor_move_cursor_visual(KEY_RIGHT,
                    I->repeat * g_config->bytes_group); break;
                case 'b': editor_move_cursor_visual(KEY_LEFT,
                    I->repeat * g_config->bytes_group); break;

                case 'r': editor_replace_visual(); break;

                case KEY_DEL:
                case 'x': editor_delete_visual(); break;
                // EOF
                case 'G':
                    if(I->repeat != 1){
                        editor_cursor_offset_scroll(I->repeat);
                        editor_update_visual_selection();
                    }else{
                        editor_cursor_offset_scroll(I->content_length-1);
                        editor_update_visual_selection();
                    }
                    break;
                case 'g':
                    editor_render_command("g");
                    c = read_key();
                    if(c == 'g'){
                        editor_cursor_offset_scroll(0);
                        editor_update_visual_selection();
                    }
                    break;
                // Start of line
                case '0': I->cursor_x = 0;
                          editor_update_visual_selection(); break;
                // End of line
                case '$': editor_move_cursor(KEY_RIGHT,
                          I->bytes_per_line-1 - I->cursor_x);
                          editor_update_visual_selection(); break;
            }
        }

        return;
    }

    // REPLACE, INSERT, CURSOR, APPEND

    // if not in ascii check for valid hex
    if (!I->in_ascii) {
        if(!isxdigit(c) && c != KEY_ESC) {
            if (isprint(c)) {
                editor_set_status(STATUS_WARNING,
                    "\"%c\" is not a valid hex character", c);
            }
            return;
        }
    }

    if(I->mode == MODE_REPLACE){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //I->repeat--;
            editor_replace_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_replace_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(I->mode == MODE_INSERT){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //I->repeat--;
            editor_insert_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_insert_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(I->mode == MODE_APPEND){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //I->repeat--;
            editor_append_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_append_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(I->mode == MODE_CURSOR){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            editor_set_mode(MODE_NORMAL);
        }else{
        }

        return;
    }

}

void editor_resize(){

    // Get new terminal size
    term_get_size(&I->screen_rows, &I->screen_cols);
    // New bytes per line
    editor_calculate_bytes_per_line();
    // Redraw the screen
    term_clear_screen();
    editor_refresh_screen();
}

void editor_load_config_file() {

    char *homedir;
    int c = 0;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    if (homedir != NULL){
        char* config_path = malloc(strlen(homedir) + 10);
        strcpy(config_path, homedir);
        strcpy(config_path + strlen(homedir), "/.hedrc");
        if( access( config_path, F_OK ) != -1 ) {
            char *buff = config_open(config_path);
            int ret = config_parse(g_config, buff);
            if (ret != -1) {
                term_set_format(FG_RED);
                printf("Error on line %d (%s)\r\n", ret+1, config_path);
                term_set_format(FORMAT_RESET);
                printf("Press ENTER to continue\r\n");
                fflush(stdout);
                while( c != KEY_ENTER) {
                    c = read_key();
                }
                term_set_format(FORMAT_RESET);
            }
		} else {
			fprintf(stderr, "Config file not found");
		}
    }
}

void editor_init(HEDConfig* conf){
    I = malloc(sizeof(HEState));
    g_config = conf;
    // Gets the terminal size
    term_get_size(&I->screen_rows, &I->screen_cols);
    // Enter in raw mode
    term_enable_raw(&I->term_original);
    // Initialize the screen buffer
    I->buff = buff_create();

    // Start buffer with 1024 HEDBytes
    I->content = malloc(1024*sizeof(HEDByte));
    I->file_name = malloc(128);
    I->file_name[0] = 0;

    // Set HEState variables
    I->scrolled = 0;

    // status bar & mode
    I->mode = MODE_NORMAL;
    I->status_message = buff_create();

    // Read command
    I->read_buff = buff_create();
    I->search_buff = buff_create();
    I->last_found.start = -1;
    I->last_found.end = -1;

    // Write
    I->last_byte = (byte_t){0};
    I->last_write_offset = -1;
    I->repeat_buff = buff_create();
    I->repeat = 1;

    I->action_list = action_list_init();
    I->in_ascii = false;

    I->grammars = grammar_list_create();

    I->selection.start = -1;
    I->selection.end   = -1;
    I->selection.is_backwards   = false;

    I->cursor_y = 1;
    I->cursor_x = 0;

    I->dirty = false;
    I->read_only = false;
    I->warned_read_only = false;

    editor_load_config_file();

    // New bytes per line
    editor_calculate_bytes_per_line();

    term_clear_screen();
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
        if (I->content != NULL){
            // Free the read file content
            free(I->content);
        }
        if (I->file_name != NULL){
            // Free the filename
            free(I->file_name);
        }
        if (I->status_message != NULL){
            // Free the status_mesasge
            free(I->status_message);
        }
        if (I->action_list != NULL){
            // Free the status_mesasge
            //TODO: free the double linked list
            free(I->action_list);
        }

        term_clear_screen();

        // Exit raw mode
        term_disable_raw(&I->term_original);

        free(I);
    }
}
