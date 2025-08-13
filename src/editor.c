#include <assert.h>
#include <limits.h>
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

static HEState g_hestate;
static HEDConfig g_config;

#define LEN_OFFSET 9
#define PADDING 1
#define MAX_COMMAND 32
#define MAX_STATUS_BAR 128

void editor_set_status(enum status_message type, const char *fmt, ...){

    char buff[MAX_STATUS_BAR];

    // We control the len on print
    buff_clear_dirty(g_hestate.status_message);
    // Clear line
    term_clear_line_end_buff(g_hestate.status_message);

    switch(type){
        case STATUS_INFO:
            term_set_format_buff(g_hestate.status_message, FG_BLUE); break;
        case STATUS_WARNING:
            term_set_format_buff(g_hestate.status_message, FG_YELLOW); break;
        case STATUS_ERROR:
            term_set_format_buff(g_hestate.status_message, FG_RED); break;
    }

    va_list ap;
    va_start(ap, fmt);
    int x = vsnprintf(buff, MAX_STATUS_BAR, fmt, ap);
    va_end(ap);

    buff_append(g_hestate.status_message, buff, x);

    term_set_format_buff(g_hestate.status_message, FORMAT_RESET);

}

// Opens the file and stores the content
void editor_open_file(char *filename){
    if (filename == NULL) {
        return;
    }
    struct stat fileinfo;
    stat(filename, &fileinfo);
    size_t size = fileinfo.st_size;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL){
        editor_set_status(STATUS_ERROR, "Cannot open file");
        return;
    }

    g_hestate.init_msg = true;

    g_hestate.content = realloc(g_hestate.content, size*sizeof(HEDByte));

    strncpy(g_hestate.file_name, filename, 128);

    g_hestate.content_length = size;

    uint8_t c[4096];
    size_t pos = 0;
    while (pos < size) {
        size_t count = fread(c, sizeof(uint8_t), sizeof(c), fp);
        if (count != 4096 && count < size - pos) {
            if (ferror(fp)) {
                editor_set_status(STATUS_ERROR, "Failed to read file: %s",
                    strerror(errno));
            }
            fclose(fp);
            return;
        }
        for(size_t i = 0; i < count; i++){
            g_hestate.content[pos].o.value = c[i];
            g_hestate.content[pos].c.value = c[i];
            g_hestate.content[pos].is_original = true;
            g_hestate.content[pos].g = -1;
            ++pos;
        }
    }

    // Check if the file is readonly, and warn the user about that.
    if (access(filename, W_OK) == -1) {
        editor_set_status(STATUS_WARNING, "\"%s\" (%d bytes) [readonly]",
            g_hestate.file_name, g_hestate.content_length);
        g_hestate.read_only = true;
    } else {
        editor_set_status(STATUS_INFO, "\"%s\" (%d bytes)",
            g_hestate.file_name, g_hestate.content_length);
        g_hestate.read_only = false;
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
        strncpy(g_hestate.file_name, name, 128);
    } else {
        // If no argument try to save to the previous opened file, if any
        if (strlen(g_hestate.file_name) == 0) {
            editor_set_status(STATUS_ERROR, "No file name");
            return;
        } else {
            fp = fopen(g_hestate.file_name, "wb");
            if (fp == NULL){
                editor_set_status(STATUS_ERROR,
                    "Unable to open '%s' for writing: %s",
                    g_hestate.file_name, strerror(errno));
                return;
            }
        }
    }



    size_t i;
    for(i = 0; i < g_hestate.content_length; i++){
        fwrite(&g_hestate.content[i].c, 1, 1, fp);
    }

    editor_set_status(STATUS_INFO, "written %zu bytes", i);

    g_hestate.dirty = false;

    fclose(fp);
}

void editor_calculate_bytes_per_line(void){

    // Calculate the maximum hex+ascii bytes that fits in the screen size
    // one byte = 2 chars
    int bytes_per_line = g_config.bytes_group; // Atleast write one byte group
    unsigned int filled;

    while((bytes_per_line / g_config.bytes_group) < g_config.groups_per_line)
    {
        bytes_per_line += g_config.bytes_group;
        filled = 10 + bytes_per_line * 2
            + (bytes_per_line/g_config.bytes_group) * PADDING + bytes_per_line;
        if (filled >= g_hestate.screen_cols){
            bytes_per_line -= g_config.bytes_group;
            break;
        }

    }

    g_hestate.bytes_per_line = bytes_per_line;
}

void editor_command_set_run(HEDBuff* cmd) {
    HEDBuff* key = buff_create();
    HEDBuff* value = buff_create();

    if (cmd->len > 0){
        config_parse_line_key_value(cmd, key, value);
        if (key->len > 0){
            int ret = config_update_key_value(&g_config, key, value);
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

int editor_close_file(void) {
    if(g_hestate.dirty){
        editor_set_status(STATUS_ERROR, "No write since last change");
        return 0;
    }

    g_hestate.file_name[0] = 0;

    g_hestate.content_length = 0;

    return 1;
}


void editor_render_ascii(unsigned int row, unsigned int start, unsigned int len){

    HEDBuff* buff = g_hestate.buff;
    int offset = start;

    for(unsigned int i = 0; i < len; i++){
        // Get byte to write
        offset = start + i;
        if(g_hestate.content_length > 0) {
        }

        // Found match
        if(offset >= g_hestate.last_found.start && offset <= g_hestate.last_found.end){
            term_set_format_buff(buff, BG_BLUE);
        }

        // Selection
        if(offset >= g_hestate.selection.start && offset <= g_hestate.selection.end){
            term_set_format_buff(buff, BG_LIGHT_GRAY);
        }

        // Cursor
        if((unsigned int)g_hestate.cursor_y == row){
            if((unsigned int)g_hestate.cursor_x == i){
                if(g_hestate.in_ascii){
                    term_set_format_buff(buff, BG_YELLOW);
                    term_set_format_buff(buff, FG_BLACK);
                }else{
                    term_set_format_buff(buff, FORMAT_INVERTED);
                }
            }
        }

        if(g_hestate.content_length > 0) {
            HEDByte *c = &g_hestate.content[offset];
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
    while(len < g_hestate.bytes_per_line){
        buff_append(buff, " ", 1);
        len++;
    }

    if(g_hestate.in_ascii == true){
        term_set_format_buff(buff, FG_YELLOW);
        buff_append(buff, "|", 1);
        term_set_format_buff(buff, FORMAT_RESET);
    }else{
        buff_append(buff, " ", 1);
    }
}


unsigned int editor_offset_at_cursor(void){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    unsigned int offset = (g_hestate.cursor_y - 1 + g_hestate.scrolled) *
                          (g_hestate.bytes_per_line) + (g_hestate.cursor_x);
    if (offset <= 0) {
        return 0;
    }
    if (offset >= g_hestate.content_length) {
        return g_hestate.content_length - 1;
    }
    return offset;
}

void editor_cursor_offset(unsigned int offset){
    // cursor_y goes from 1 to ..., cursor_x goes from 0 to bytes_per_line
    g_hestate.cursor_x = offset % g_hestate.bytes_per_line;
    g_hestate.cursor_y = offset / g_hestate.bytes_per_line - g_hestate.scrolled + 1;
}

void editor_check_scroll_top_limit(void){
    // editor_calculate_bytes_per_line shouldv'e already been called, but assert just in case
    assert(g_hestate.bytes_per_line);
    // max scroll 
    unsigned int top_limit = g_hestate.content_length/g_hestate.bytes_per_line
                             - (g_hestate.screen_rows-3);
    if(g_hestate.scrolled >= top_limit){
        g_hestate.scrolled = top_limit;
    }
}

void editor_cursor_offset_scroll(unsigned int offset){
    if(offset > g_hestate.content_length){
        // Out of bounds
        return;
    }

    // Check if offset is in view range
    unsigned int offset_min = g_hestate.scrolled * g_hestate.bytes_per_line;
	unsigned int offset_max = offset_min + ((g_hestate.screen_rows-1)
                              * g_hestate.bytes_per_line);

    if (offset >= offset_min && offset <= offset_max) {
        editor_cursor_offset(offset);
        return;
    }

    g_hestate.scrolled = (int) (offset / g_hestate.bytes_per_line - ((g_hestate.screen_rows-2)/2));

    if (g_hestate.scrolled <= 0) {
        g_hestate.scrolled = 0;
    }

    editor_check_scroll_top_limit();

    editor_cursor_offset(offset);
}


void editor_scroll(int units) {

    g_hestate.scrolled += units;

    editor_check_scroll_top_limit();

}

void editor_move_cursor(int dir, int amount){

    // Don't move if file empty
    if(g_hestate.content_length == 0){
        return;
    }

    for(int i = 0; i < amount; i++){
        switch (dir){
            case KEY_UP: g_hestate.cursor_y--; break;
            case KEY_DOWN: g_hestate.cursor_y++; break;
            case KEY_LEFT: g_hestate.cursor_x--; break;
            case KEY_RIGHT: g_hestate.cursor_x++; break;
        }

        // Beggining of file, stop
        if(g_hestate.cursor_x <= 0 && g_hestate.cursor_y <= 1 && g_hestate.scrolled <= 0){
            g_hestate.cursor_x = 0;
            g_hestate.cursor_y = 1;
            return;
        }

        // Stop left limit
        if(g_hestate.cursor_x < 0){
            // If not the first line, move up
            if(g_hestate.cursor_y > 1){
                g_hestate.cursor_y--;
                g_hestate.cursor_x = g_hestate.bytes_per_line-1;
            }else{
                g_hestate.cursor_x = 0;
            }
        }

        // Stop right limit
        if(g_hestate.cursor_x >= (int)g_hestate.bytes_per_line){
            g_hestate.cursor_y++;
            g_hestate.cursor_x = 0;
        }

        // Stop top file limit
        if(g_hestate.cursor_y <= 1 && g_hestate.scrolled <= 0){
            g_hestate.cursor_y = 1;
        }

        if((unsigned int)g_hestate.cursor_y > g_hestate.screen_rows - 2){
            g_hestate.cursor_y = g_hestate.screen_rows - 2;
            editor_scroll(1);
        }else if (g_hestate.cursor_y < 1 && g_hestate.scrolled > 0){
            g_hestate.cursor_y = 1;
            editor_scroll(-1);
        }

        unsigned int offset = editor_offset_at_cursor();

        if (offset >= g_hestate.content_length - 1) {
            editor_cursor_offset(offset);
        }
    }

}

void editor_update_visual_selection(void){

    unsigned int offset = editor_offset_at_cursor();
    assert(offset <= INT_MAX);

    // If we moving backwards, the start of selection
    if((int)offset <= g_hestate.selection.start && !g_hestate.selection.is_backwards){
        g_hestate.selection.end          = g_hestate.selection.start;
        g_hestate.selection.is_backwards = true;
    }else if((int)offset >= g_hestate.selection.end && g_hestate.selection.is_backwards){
    // If we are crossing forwards again
        g_hestate.selection.start        = g_hestate.selection.end;
        g_hestate.selection.is_backwards = false;
    }

    if(g_hestate.selection.is_backwards){
        g_hestate.selection.start = offset;
    }else{
        g_hestate.selection.end = offset;
    }
}

void editor_move_cursor_visual(int dir, int amount){

    editor_move_cursor(dir, amount);

    editor_update_visual_selection();

}

void editor_render_content(HEDBuff* buff){

    unsigned int line_chars = 0;
    unsigned int line_bytes = 0;
    unsigned int chars_until_ascii = 0;

    unsigned int line_offset_start = 0;

    unsigned int row = 0;

    off_t offset;

    chars_until_ascii = 10 + g_hestate.bytes_per_line * 2 + g_hestate.bytes_per_line/2;

    unsigned int offset_start = g_hestate.scrolled * g_hestate.bytes_per_line;

    if(offset_start >= g_hestate.content_length){
        offset_start = g_hestate.content_length;
    }

    // ruler + status
    unsigned int bytes_per_screen = g_hestate.bytes_per_line * (g_hestate.screen_rows - 2);
    unsigned int offset_end = bytes_per_screen + offset_start;

    // Don't show more than content_length
    if(offset_end > g_hestate.content_length ){
        offset_end = g_hestate.content_length;
    }

    // Empty file
    if(offset_end == 0){
        offset_end ++;
    }

    for (offset = offset_start; offset < offset_end; offset++){
        // New row
        if(offset % g_hestate.bytes_per_line == 0 ){
            line_chars = 0;
            line_bytes = 0;
            line_offset_start = offset;
            term_set_format_buff(buff, FG_BLUE);
            buff_vappendf(buff, "%08x: ", offset);
            line_chars += 10;
            row++;
            // Cursor side
            if(g_hestate.in_ascii == false){
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
        if(offset >= g_hestate.last_found.start && offset <= g_hestate.last_found.end){
            term_set_format_buff(buff, BG_BLUE);
        }

        // Selection
        if(offset >= g_hestate.selection.start && offset <= g_hestate.selection.end){
            term_set_format_buff(buff, BG_LIGHT_GRAY);
        }

        // Cursor
        if((unsigned int)g_hestate.cursor_y == row){
            if((unsigned int)g_hestate.cursor_x == line_bytes){
                if(g_hestate.in_ascii){
                    term_set_format_buff(buff, FORMAT_INVERTED);
                }else{
                    term_set_format_buff(buff, BG_YELLOW);
                    term_set_format_buff(buff, FG_BLACK);
                }
            }
        }

        if(g_hestate.content_length == 0){
            buff_vappendf(buff, "%02x", 0);
        }else{

            // Grammar color
            enum color_bg color = grammar_color_id(g_hestate.grammars,
                g_hestate.content[offset].g);
            if(color != COLOR_NOCOLOR){
                term_set_format_buff(buff, color);
            }

            // Write the value on the screen (HEDBuff)
            // If the value is changed
            if(g_hestate.content[offset].c.value != g_hestate.content[offset].o.value
                || !g_hestate.content[offset].is_original){
                term_set_format_buff(buff, FG_RED);
            }

            buff_vappendf(buff, "%02x",
                (unsigned int) g_hestate.content[offset].c.value);

        }

        line_chars += 2;
        line_bytes += 1;
        // Reset color
        term_set_format_buff(buff, FORMAT_RESET);

        // Every group, write a separator of len PADDING, unless its the
        // last in line or the last row
        if((line_bytes % g_config.bytes_group == 0) &&
            (line_bytes != g_hestate.bytes_per_line)){
            for(int s=0; s < PADDING; s++){
                buff_append(buff, " ", 1);
                line_chars += 1;
            }
        }
        // If end of line, write ascii and new line
        if((offset + 1) % g_hestate.bytes_per_line == 0){
            // Cursor side
            term_set_format_buff(buff, FG_YELLOW);
            if(g_hestate.in_ascii == false){
                buff_append(buff, "| ", 2);
            }else{
                buff_append(buff, " |", 2);
            }
            line_chars += 2;
            // End Cursor side
            term_set_format_buff(buff, FORMAT_RESET);
            editor_render_ascii(row, line_offset_start, g_hestate.bytes_per_line);
            line_chars += g_hestate.bytes_per_line; // ascii chars
            term_clear_line_end_buff(buff);
            buff_append(buff, "\r\n", 2);
        }
    }

    // If we need padding for ascii
    if(offset % g_hestate.bytes_per_line != 0){
        // Fill until the ascii chars in case of short line
        while( line_chars < chars_until_ascii){
                buff_append(buff, " ", 1);
                line_chars++;
        }
        term_set_format_buff(buff, FG_YELLOW);
        if(g_hestate.in_ascii == false){
            buff_append(buff, "| ", 2);
        }else{
            buff_append(buff, " |", 2);
        }
        term_set_format_buff(buff, FORMAT_RESET);
        // Render ascii
        editor_render_ascii(row, line_offset_start, line_bytes);
        term_clear_line_end_buff(buff);
    }


    while(++row <= g_hestate.screen_rows - 2){
        // clear everything up until the status bar and ruler
        term_goto_buff(buff, row, 0);
        term_clear_line_end_buff(buff);
    }


}

void editor_render_command(char *command){
    char buffer[32];
    term_goto(g_hestate.screen_rows-EDITOR_COMMAND_STATUS_OFFSET_Y,
        g_hestate.screen_cols-EDITOR_COMMAND_STATUS_OFFSET_X);
    int w = sprintf(buffer, "%s", command);
    term_print(buffer, w);
}

// Modes

void editor_start_mode_normal(void){
    // Reset selection
    g_hestate.selection.start = -1;
    g_hestate.selection.end = -1;
}

void editor_start_mode_replace(void){
    if (g_hestate.read_only && !g_hestate.warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        g_hestate.warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(g_hestate.repeat_buff->len != 0){
        g_hestate.repeat_buff->len = 0;
    }
}

void editor_start_mode_insert(void){
    if (g_hestate.read_only && !g_hestate.warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        g_hestate.warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(g_hestate.repeat_buff->len != 0){
        g_hestate.repeat_buff->len = 0;
    }
}

void editor_start_mode_visual(void){
    unsigned int offset = editor_offset_at_cursor();
    g_hestate.selection.start = offset;
    g_hestate.selection.end   = offset;
}

void editor_start_mode_cursor(void){
}

void editor_start_mode_command(void){
}

void editor_start_mode_append(void) {
    if (g_hestate.read_only && !g_hestate.warned_read_only) {
        editor_set_status(STATUS_WARNING, "Changing a readonly file");
        g_hestate.warned_read_only = true;
    }
    // If we have data in the repeat buffer, start from the index 0 again
    if(g_hestate.repeat_buff->len != 0){
        g_hestate.repeat_buff->len = 0;
    }
}

void editor_set_mode(enum editor_mode mode){
    editor_set_status(STATUS_INFO, "");
    g_hestate.mode = mode;
    switch(g_hestate.mode){
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

void editor_render_ruler_left(HEDBuff* buff){
    // Left ruler
    // Start writting the text now
    term_goto_buff(buff, g_hestate.screen_rows - EDITOR_RULER_OFFSET_Y, 0);

    // Write the left ruler
    if(strlen(g_hestate.file_name) != 0){
        buff_vappendf(buff, "%s ", g_hestate.file_name);
    }else{
        buff_vappendf(buff, "[No Name]");
    }

    if(g_hestate.dirty){
        buff_append(buff, "[+]", 3);
    }
    if(g_hestate.read_only){
        buff_append(buff, "[RO]", 4);
    }
}

void editor_render_ruler_right(HEDBuff* buff){
    unsigned int offset = editor_offset_at_cursor();
    // If no data percentage will be 100%
    unsigned int percentage = 100;

    // Right ruler position
    term_goto_buff(buff, g_hestate.screen_rows-1,
                          g_hestate.screen_cols - EDITOR_RULER_RIGHT_OFFSET_X);

    if(g_hestate.content_length > 1){
        percentage = offset*100/(g_hestate.content_length-1);
    }

    // Write the right ruler
    buff_vappendf(buff, "0x%08x (%d)  %d%%", offset, offset, percentage);
}

void editor_render_ruler(HEDBuff* buff) {

    // Move to position and clear line
    term_goto_buff(buff, g_hestate.screen_rows - EDITOR_RULER_OFFSET_Y, 0);
    term_clear_line_buff(buff);

    // Write the line background and foreground colors
    term_set_format_buff(buff, BG_LIGHT_GRAY);
    term_set_format_buff(buff, FG_BLACK);

    // Create an empty line colored with the background
    for(unsigned int i = 0; i < g_hestate.screen_cols; i++){
        buff_append(buff, " ", 1);
    }

    editor_render_ruler_left(buff);
    editor_render_ruler_right(buff);

    term_clear_line_end_buff(buff);
    term_set_format_buff(buff, FORMAT_RESET);
}

void editor_render_status_mode(HEDBuff* buff) {

    term_set_format_buff(buff, FG_GREEN);

    switch(g_hestate.mode){
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
    term_goto_buff(buff, g_hestate.screen_rows, 0);

    editor_render_status_mode(buff);

    unsigned int len = g_hestate.status_message->len;
    if (g_hestate.screen_cols <= len) {
        len = g_hestate.screen_cols;
        buff_append(buff, g_hestate.status_message->content, len-3);
        buff_append(buff, "...", 3);
    }
    buff_append(buff, g_hestate.status_message->content, len);
    term_clear_line_end_buff(buff);
}

void editor_refresh_screen(void){

    HEDBuff* buff = g_hestate.buff;

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
    if (!g_hestate.content_length && !g_hestate.init_msg) {
        term_goto_buff(buff, g_hestate.screen_rows/2 - 4, g_hestate.screen_cols/2 - 5);
        buff_vappendf(buff, "HED - hex editor");
        term_goto_buff(buff, g_hestate.screen_rows/2 - 2, g_hestate.screen_cols/2 - 3);
        buff_vappendf(buff, "version %s", HED_VERSION_SHORT);
        term_goto_buff(buff, g_hestate.screen_rows/2, g_hestate.screen_cols/2 - 5);
        buff_vappendf(buff, "by Ferran Celades");
        fflush(stdout);
    }

    // Move to the last line of the screen
    term_goto_buff(buff, g_hestate.screen_rows, 1);

    // Write the buffer on the screen
    term_print(buff->content, buff->len);
}

void editor_write_byte_offset(unsigned char new_byte, unsigned int offset){

    g_hestate.content[offset].c.value = new_byte;
}

void editor_prepare_write_repeat(char ch){

    if((g_hestate.repeat_buff->len+1 >= g_hestate.repeat_buff->capacity)){
        g_hestate.repeat_buff->content = realloc(g_hestate.repeat_buff->content, g_hestate.repeat_buff->capacity*2);
        g_hestate.repeat_buff->capacity *= 2;
    }
    g_hestate.repeat_buff->content[g_hestate.repeat_buff->len] = ch;
    g_hestate.repeat_buff->len++;
}

void editor_replace_offset(unsigned int offset, unsigned char c){

    HEDByte old_byte;

    if (!g_hestate.content_length) {
        editor_set_status(STATUS_WARNING, "File is empty, nothing to replace");
        return;
    }

    if(offset >= g_hestate.content_length){
        return;
    }

    g_hestate.dirty = true;

    if(g_hestate.in_ascii){
        // Create the action
        // Original, Current, grammar
        HEDByte action_byte = {{g_hestate.content[offset].c.value},
                               {c},
                               g_hestate.content[offset].is_original,
                               g_hestate.content[offset].g};
        action_add(g_hestate.action_list, ACTION_REPLACE, offset, action_byte,
            g_hestate.repeat);
        editor_write_byte_offset(c, offset);
        editor_move_cursor(KEY_RIGHT, 1);
        return;
    }

    // Second octet
    if(g_hestate.last_write_offset == offset){
        old_byte = g_hestate.content[offset];
        // One octet already written in this position

        if (g_config.replace_nibble == 0) {
            g_hestate.content[offset].c.nibble.top =
                    g_hestate.content[offset].c.nibble.bottom;
            g_hestate.content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            g_hestate.content[offset].c.nibble.bottom = utils_hex2int(c);
        }

        if (old_byte.c.value != g_hestate.content[offset].c.value) {
            g_hestate.content[offset].is_original = false;
        }
        // Modify the last action to reflect the 2nd nibble
        g_hestate.action_list->last->b.c.value = g_hestate.content[offset].c.value;

        editor_move_cursor(KEY_RIGHT, 1);

        g_hestate.last_write_offset = -1;
    }else{

        old_byte = g_hestate.content[offset];

        if (g_config.replace_nibble == 0) {
            g_hestate.content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            g_hestate.content[offset].c.nibble.top = utils_hex2int(c);
        }

        if (old_byte.c.value != g_hestate.content[offset].c.value) {
            g_hestate.content[offset].is_original = false;
        }

        g_hestate.last_write_offset = offset;

        // Create the action
        HEDByte action_byte = {{old_byte.c.value},
                                {g_hestate.content[offset].c.value},
                                old_byte.is_original,
                                g_hestate.content[offset].g};
        action_add(g_hestate.action_list, ACTION_REPLACE, offset, action_byte, g_hestate.repeat);
    }

}

void editor_replace_cursor(char c){

    unsigned int offset = editor_offset_at_cursor();
    editor_replace_offset(offset, c);

}

void editor_replace_repeat_last(int times){
    g_hestate.repeat = g_hestate.repeat_buff->len * times;
    if (!g_hestate.in_ascii) {
        g_hestate.repeat /= 2;
    }
    for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
        editor_replace_cursor(g_hestate.repeat_buff->content[c]);
    }

}


void editor_replace_cursor_repeat(void){

    for(unsigned int r=0; r < g_hestate.repeat - 1; r++){
        for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
            editor_replace_cursor(g_hestate.repeat_buff->content[c]);
        }
    }
}

void editor_replace_visual(void){

    int c = read_key();

    // action repeat
    g_hestate.repeat = g_hestate.selection.end - g_hestate.selection.start + 1;

    // No need to scroll
    editor_cursor_offset(g_hestate.selection.start);

    if(isxdigit(c)){
        if(g_hestate.in_ascii){
            for(int i= g_hestate.selection.start; i <= g_hestate.selection.end; i++){
                editor_replace_offset(i, c);
            }
        }else{
            int c2 = read_key();
            if(isxdigit(c2)){
                for(int i= g_hestate.selection.start; i <= g_hestate.selection.end; i++){
                    editor_replace_offset(i, c); // First octet
                    editor_replace_offset(i, c2); // Second octet
                }

            }else{
                for(int i= g_hestate.selection.start; i <= g_hestate.selection.end; i++){
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

    if(offset >= g_hestate.content_length && g_hestate.content_length != 0){
        return;
    }

    g_hestate.dirty = true;
    g_hestate.init_msg = true;

    if(g_hestate.in_ascii){
        g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length + 1)*sizeof(HEDByte));
        memmove(&g_hestate.content[offset+1], &g_hestate.content[offset],
            (g_hestate.content_length - offset)*sizeof(HEDByte));
        g_hestate.content_length++;

        // Make sure to set the original to 0 on insert
        g_hestate.content[offset].o.value = 0;
        g_hestate.content[offset].is_original = false;
        // Less significative first
        g_hestate.content[offset].c.value = c;

        HEDByte action_byte = {{0},{c},false,0};

        // Create the action
        action_add(g_hestate.action_list, ACTION_INSERT, offset, action_byte, g_hestate.repeat);
        editor_move_cursor(KEY_RIGHT, 1);
        return;
    }

    // Second octet
    if(g_hestate.last_write_offset == offset){

        if (g_config.insert_nibble == 0) {
            g_hestate.content[offset].c.nibble.top =
                        g_hestate.content[offset].c.nibble.bottom;
            g_hestate.content[offset].c.nibble.bottom = utils_hex2int(c);
        } else {
            g_hestate.content[offset].c.nibble.bottom = utils_hex2int(c);
        }

        // Modify the last action to reflect the 2nd nibble
        g_hestate.action_list->last->b.c.value = g_hestate.content[offset].c.value;

        if (append == false) {
            editor_move_cursor(KEY_RIGHT, 1);
        }

        g_hestate.last_write_offset = -1;

    }else{
        // If we append, add offset 1, unless the file is empty
        if (append && g_hestate.content_length > 0) {
            offset++;
        }
        // First octet
        // Increase allocation by one byte_t
        // TODO: only if not space already
        g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length + 1)
                             * sizeof(HEDByte));
        memmove(&g_hestate.content[offset+1], &g_hestate.content[offset],
                (g_hestate.content_length - offset)*sizeof(HEDByte));
        g_hestate.content_length++;

        new_byte = utils_hex2int(c);

        // Make sure to set the original to 0 and on insert
        g_hestate.content[offset].o.value = 0;
        g_hestate.content[offset].c.value = 0;
        g_hestate.content[offset].is_original = false;

        if (g_config.insert_nibble == 0) {
            // Less significative first
            g_hestate.content[offset].c.nibble.bottom = new_byte;
        } else {
            g_hestate.content[offset].c.nibble.top = new_byte;
        }

        g_hestate.last_write_offset = offset;

        HEDByte action_byte = {{0}, {new_byte}, false, 0};

        if (append && g_hestate.content_length > 0) {
            editor_move_cursor(KEY_RIGHT, 1);
        }

        if (append) {
            action_add(g_hestate.action_list, ACTION_APPEND, offset, action_byte, g_hestate.repeat);
        } else {
            action_add(g_hestate.action_list, ACTION_INSERT, offset, action_byte, g_hestate.repeat);
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
    g_hestate.repeat = g_hestate.repeat_buff->len * times;
    if (!g_hestate.in_ascii) {
        g_hestate.repeat /= 2;
    }
    for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
        editor_append_cursor(g_hestate.repeat_buff->content[c]);
    }
}

void editor_append_cursor_repeat(void) {
    for(unsigned int r=0; r < g_hestate.repeat - 1; r++){
        for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
            editor_append_cursor(g_hestate.repeat_buff->content[c]);
        }
    }
}

void editor_insert_repeat_last(int times) {
    g_hestate.repeat = g_hestate.repeat_buff->len * times;
    if (!g_hestate.in_ascii) {
        g_hestate.repeat /= 2;
    }
    for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
        editor_insert_cursor(g_hestate.repeat_buff->content[c]);
    }

}

void editor_insert_cursor_repeat(void) {
    for(unsigned int r=0; r < g_hestate.repeat - 1; r++){
        for(unsigned int c=0; c < g_hestate.repeat_buff->len; c++){
            editor_insert_cursor(g_hestate.repeat_buff->content[c]);
        }
    }
}

void editor_undo_insert_offset(unsigned int offset){
    memmove(&g_hestate.content[offset], &g_hestate.content[offset+1],
        (g_hestate.content_length - offset-1)*sizeof(HEDByte));
    g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length - 1) * sizeof(HEDByte));
    g_hestate.content_length--;
    g_hestate.dirty = true;
}

void editor_redo_insert_offset(unsigned int offset, HEDByte b){
    g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length + 1) * sizeof(HEDByte));
    memmove(&g_hestate.content[offset+1], &g_hestate.content[offset],
        (g_hestate.content_length - offset)*sizeof(HEDByte));
    g_hestate.content[offset].c = b.c;
    g_hestate.content[offset].o = b.o;
    g_hestate.content[offset].is_original = b.is_original;
    g_hestate.content_length++;
    g_hestate.dirty = true;
}

void editor_undo_delete_offset(unsigned int offset, HEDByte b){
    editor_redo_insert_offset(offset, b);
    g_hestate.dirty = true;
}

void editor_delete_offset(unsigned int offset) {
    if(g_hestate.content_length > 0){
        HEDByte old_byte = g_hestate.content[offset];
        memmove(&g_hestate.content[offset], &g_hestate.content[offset+1],
            (g_hestate.content_length - offset-1)*sizeof(HEDByte));
        g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length - 1)
            * sizeof(HEDByte));
        HEDByte action_byte = {{old_byte.c.value},
                                {old_byte.o.value},
                                old_byte.is_original ,
                                old_byte.g};
        action_add(g_hestate.action_list, ACTION_DELETE, offset, action_byte, g_hestate.repeat);
        g_hestate.content_length--;
    } else {
        editor_set_status(STATUS_WARNING, "File is empty");
    }
    g_hestate.dirty = true;
}

void editor_delete_cursor(void){
    unsigned int offset = editor_offset_at_cursor();
    editor_delete_offset(offset);
    if(offset > 0 && offset == g_hestate.content_length){
        editor_move_cursor(KEY_LEFT, 1);
    }else{
        editor_cursor_offset(offset);
    }
    g_hestate.dirty = true;
}

void editor_delete_cursor_repeat(void){
    for(unsigned int r=0; r < g_hestate.repeat; r++){
        editor_delete_cursor();
    }
}

void editor_delete_visual(void){
    g_hestate.repeat = g_hestate.selection.end - g_hestate.selection.start + 1;
    for(int i= g_hestate.selection.start; i <= g_hestate.selection.end; i++){
        editor_delete_offset(g_hestate.selection.start);
    }
    editor_cursor_offset(g_hestate.selection.start);
    editor_set_mode(MODE_NORMAL);
    g_hestate.dirty = true;
}


void editor_reset_write_repeat(void){
    g_hestate.last_write_offset = -1;
}

void editor_redo_delete_offset(unsigned int offset){
    memmove(&g_hestate.content[offset], &g_hestate.content[offset+1],
        (g_hestate.content_length - offset-1)*sizeof(HEDByte));
    g_hestate.content = realloc(g_hestate.content, (g_hestate.content_length - 1)*sizeof(HEDByte));
    g_hestate.content_length--;
    editor_cursor_offset(offset);
    g_hestate.dirty = true;
}

void editor_undo_redo_replace_offset(unsigned int offset, HEDByte b){
    HEActionList *list = g_hestate.action_list;
    // Swap original and current in action
    list->current->b.o = g_hestate.content[offset].c;
    list->current->b.c = g_hestate.content[offset].o;
    list->current->b.is_original = g_hestate.content[offset].is_original;

    g_hestate.content[offset].c = b.o;
    g_hestate.content[offset].is_original = b.is_original;
    g_hestate.dirty = true;
}


void editor_repeat_last_action(void){

    HEActionList *list = g_hestate.action_list;
    int times = g_hestate.repeat;
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

void editor_redo(void){

    HEActionList *list = g_hestate.action_list;

    // If newest change
    if(list->current->next == NULL){
        editor_set_status(STATUS_INFO, "Already at newest change");
        return;
    }

    for(unsigned int i = 0; i < g_hestate.repeat; i++){

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
                    editor_redo_delete_offset(offset); break;
                case ACTION_APPEND:
                    editor_redo_insert_offset(offset, b);
                    editor_cursor_offset_scroll(offset + 1);
                    break;
            }
            // if (list->current->next != NULL){
            //     list->current = list->current->next;
            // }
        }
        g_hestate.dirty = true;
    }


}

void editor_undo(void){

    HEActionList *list = g_hestate.action_list;

    if (list->current->type == ACTION_BASE) {
        editor_set_status(STATUS_INFO, "Already at oldest change");
        g_hestate.dirty = false;
        return;
    }

    for(size_t i = 0; i < g_hestate.repeat; i++){

        // Undo multiple actions
        int repeat = list->current->repeat;

        for(int r = 0; r < repeat; r++) {

            unsigned int offset = list->current->offset;
            HEDByte b = list->current->b;

            switch(list->current->type){
                case ACTION_BASE:
                    editor_set_status(STATUS_INFO, "Already at oldest change");
                    g_hestate.dirty = false;
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
        g_hestate.dirty = true;
    }

}

void editor_toggle_cursor(void){
    g_hestate.in_ascii = (g_hestate.in_ascii == true) ? false: true;
}


void editor_process_quit(bool force){

    if(force){
        exit(0);
    }

    if(g_hestate.dirty){
        editor_set_status(STATUS_ERROR, "No write since last change");
        return;
    }

    exit(0);
}

void editor_process_command(void){

    char* command = g_hestate.read_buff->content;
    int len = g_hestate.read_buff->len;

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
                    editor_command_set_run(g_hestate.read_buff);
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

    buff_clear(g_hestate.read_buff);

}


void editor_read_string(char *prompt){
    read_line(g_hestate.read_buff, prompt);
}

void editor_read_command(void){
    editor_read_string(":");
}

void editor_process_search_repeat(SEARCH_DIRECTION direction, bool is_repeat) {

    HEDBuff* search_pattern;

    if (g_hestate.in_ascii) {
        search_pattern = g_hestate.search_buff;
    } else {
        if (!utils_hexonly(g_hestate.search_buff)) {
            editor_set_status(STATUS_WARNING, "Search pattern is not hex");
            return;
        }
        search_pattern = buff_create();
        utils_hexstring_to_buff(g_hestate.search_buff, search_pattern);
    }

    int offset = editor_offset_at_cursor();

    if (is_repeat) {
        if (direction == SEARCH_FORWARD) {
            offset++;
        } else {
            offset--;
        }
    }

    int found_offset = search_buffer(g_hestate.content, g_hestate.content_length,
        search_pattern, offset, direction);


    if (found_offset != SEARCH_NOT_FOUND) {
        editor_cursor_offset_scroll(found_offset);
        editor_set_status(STATUS_INFO, "Pattern found at 0x%x", found_offset);
        g_hestate.last_found.start = found_offset;
        if (g_hestate.in_ascii) {
            g_hestate.last_found.end = found_offset + g_hestate.search_buff->len - 1;
        } else {
            g_hestate.last_found.end = found_offset + search_pattern->len - 1;
        }
    } else {
        editor_set_status(STATUS_WARNING, "Pattern not found");
        g_hestate.last_found.start = -1;
        g_hestate.last_found.end = -1;
    }

    if (!g_hestate.in_ascii) {
        buff_remove(search_pattern);
    }
}

void editor_process_search(SEARCH_DIRECTION direction) {

    term_clear_line_end();

    if (direction == SEARCH_FORWARD) {
        if (g_hestate.in_ascii) {
            read_line(g_hestate.search_buff, "(asc)/");
        } else {
            read_line(g_hestate.search_buff, "(hex)/");
        }
    } else {
        if (g_hestate.in_ascii) {
            read_line(g_hestate.search_buff, "(asc)?");
        } else {
            read_line(g_hestate.search_buff, "(hex)?");
        }
    }

    editor_process_search_repeat(direction, false);
}

void editor_increment_byte_cursor(int repeat){
    if (g_hestate.content_length > 0) {
        int offset = editor_offset_at_cursor();
        HEDByte action_byte = g_hestate.content[offset];
        action_byte.o.value = action_byte.c.value;
        g_hestate.content[offset].c.value += repeat;
        g_hestate.repeat = 1;
        action_add(g_hestate.action_list, ACTION_REPLACE, offset, action_byte, g_hestate.repeat);
    }
}

void editor_decrement_byte_cursor(int repeat){
    if (g_hestate.content_length > 0) {
        int offset = editor_offset_at_cursor();
        HEDByte action_byte = g_hestate.content[offset];
        action_byte.o.value = action_byte.c.value;
        g_hestate.content[offset].c.value -= repeat;
        g_hestate.repeat = 1;
        action_add(g_hestate.action_list, ACTION_REPLACE, offset, action_byte, g_hestate.repeat);
    }
}


// Process the key pressed
void editor_process_keypress(void){

    char command[MAX_COMMAND];
    command[0] = '\0';

    int c;

    if(g_hestate.mode == MODE_COMMAND){
        editor_read_command();
        editor_set_mode(MODE_NORMAL);
        editor_process_command();
        return;
    }

    // Read first command char
    c = read_key();

    if(g_hestate.mode == MODE_NORMAL){
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
            g_hestate.repeat = atoi(command);
            if(g_hestate.repeat <= 0 ){
                g_hestate.repeat = 1;
            }
        }

        switch (c){
            case KEY_TAB: editor_toggle_cursor(); break;

            case KEY_LEFT:
            case 'h': editor_move_cursor(KEY_LEFT, g_hestate.repeat); break;
            case KEY_DOWN:
            case 'j': editor_move_cursor(KEY_DOWN, g_hestate.repeat); break;
            case KEY_UP:
            case 'k': editor_move_cursor(KEY_UP, g_hestate.repeat); break;
            case KEY_RIGHT:
            case 'l': editor_move_cursor(KEY_RIGHT, g_hestate.repeat); break;
			
            case KEY_PAGEUP: editor_move_cursor(KEY_UP, g_hestate.screen_rows); break;
            case KEY_PAGEDOWN: editor_move_cursor(KEY_DOWN, g_hestate.screen_rows); break;

            case 'w': editor_move_cursor(KEY_RIGHT,
                g_hestate.repeat * g_config.bytes_group); break;
            case 'b': editor_move_cursor(KEY_LEFT,
                g_hestate.repeat * g_config.bytes_group); break;

            //case 'd': editor_delete_cursor_repeat(); break;

            case '[': editor_decrement_byte_cursor(g_hestate.repeat); break;
            case ']': editor_increment_byte_cursor(g_hestate.repeat); break;

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
                g_hestate.bytes_per_line-1 - g_hestate.cursor_x);
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
                if(g_hestate.repeat != 1){
                    editor_cursor_offset_scroll(g_hestate.repeat);
                }else{
                    editor_cursor_offset_scroll(g_hestate.content_length-1);
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
            case '0': g_hestate.cursor_x = 0; break;
            // End of line
            case '$': editor_move_cursor(KEY_RIGHT,
                g_hestate.bytes_per_line-1 - g_hestate.cursor_x); break;
        }

        return;
    }

    if(g_hestate.mode == MODE_VISUAL){
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
            g_hestate.repeat = atoi(command);
            if(g_hestate.repeat <= 0 ){
                g_hestate.repeat = 1;
            }
        }

        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            editor_set_mode(MODE_NORMAL);
        }else{
            switch(c){

                case KEY_LEFT:
                case 'h': editor_move_cursor_visual(KEY_LEFT, g_hestate.repeat); break;
                case KEY_DOWN:
                case 'j': editor_move_cursor_visual(KEY_DOWN, g_hestate.repeat); break;
                case KEY_UP:
                case 'k': editor_move_cursor_visual(KEY_UP, g_hestate.repeat); break;
                case KEY_RIGHT:
                case 'l': editor_move_cursor_visual(KEY_RIGHT, g_hestate.repeat); break;
			    
                case KEY_PAGEUP: editor_move_cursor(KEY_UP, g_hestate.screen_rows); break;
                case KEY_PAGEDOWN: editor_move_cursor(KEY_DOWN, g_hestate.screen_rows); break;

                case 'd': editor_delete_visual(); break;

                case 'w': editor_move_cursor_visual(KEY_RIGHT,
                    g_hestate.repeat * g_config.bytes_group); break;
                case 'b': editor_move_cursor_visual(KEY_LEFT,
                    g_hestate.repeat * g_config.bytes_group); break;

                case 'r': editor_replace_visual(); break;

                case KEY_DEL:
                case 'x': editor_delete_visual(); break;
                // EOF
                case 'G':
                    if(g_hestate.repeat != 1){
                        editor_cursor_offset_scroll(g_hestate.repeat);
                        editor_update_visual_selection();
                    }else{
                        editor_cursor_offset_scroll(g_hestate.content_length-1);
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
                case '0': g_hestate.cursor_x = 0;
                          editor_update_visual_selection(); break;
                // End of line
                case '$': editor_move_cursor(KEY_RIGHT,
                          g_hestate.bytes_per_line-1 - g_hestate.cursor_x);
                          editor_update_visual_selection(); break;
            }
        }

        return;
    }

    // REPLACE, INSERT, CURSOR, APPEND

    // if not in ascii check for valid hex
    if (!g_hestate.in_ascii) {
        if(!isxdigit(c) && c != KEY_ESC) {
            if (isprint(c)) {
                editor_set_status(STATUS_WARNING,
                    "\"%c\" is not a valid hex character", c);
            }
            return;
        }
    }

    if(g_hestate.mode == MODE_REPLACE){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //g_hestate.repeat--;
            editor_replace_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_replace_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(g_hestate.mode == MODE_INSERT){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //g_hestate.repeat--;
            editor_insert_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_insert_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(g_hestate.mode == MODE_APPEND){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            // Already one repeat write
            //g_hestate.repeat--;
            editor_append_cursor_repeat();
            editor_reset_write_repeat();
            editor_set_mode(MODE_NORMAL);
        }else{
            editor_append_cursor(c);
            editor_prepare_write_repeat(c);
        }

        return;
    }

    if(g_hestate.mode == MODE_CURSOR){
        // Finish repeat sequence and go to normal mode
        if(c == KEY_ESC){
            editor_set_mode(MODE_NORMAL);
        }else{
        }

        return;
    }

}

void editor_resize(int sig){
    // this suppresses unused parameter warnings, without doing anything
    (void)sig;
    // Get new terminal size
    term_get_size(&g_hestate.screen_rows, &g_hestate.screen_cols);
    // New bytes per line
    editor_calculate_bytes_per_line();
    // Redraw the screen
    term_clear_screen();
    editor_refresh_screen();
}

void editor_load_config_file(void) {

    char *homedir;
    int c = 0;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    if (homedir != NULL){
        char* config_path = malloc(strlen(homedir) + 10);
        if (config_path == NULL) {
            perror("Failed to allocate space for config path");
            exit(1);
        }
        strcpy(config_path, homedir);
        strcpy(config_path + strlen(homedir), "/.hedrc");
        if( access( config_path, F_OK ) != -1 ) {
            char *buff = config_open(config_path);
            int ret = config_parse(&g_config, buff);
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
        }
        free(config_path);
    }
}

void editor_init(void){
    config_init(&g_config);
    // Gets the terminal size
    term_get_size(&g_hestate.screen_rows, &g_hestate.screen_cols);
    // Enter in raw mode
    term_enable_raw(&g_hestate.term_original);
    // Initialize the screen buffer
    g_hestate.buff = buff_create();
    g_hestate.init_msg = false;

    // Start buffer with 1024 HEDBytes
    g_hestate.content = malloc(1024*sizeof(HEDByte));
    g_hestate.content_length = 0;
    g_hestate.file_name = malloc(128);
    if (g_hestate.file_name == NULL) {
        perror("Failed ot allocate space for file name");
        exit(1);
    }
    g_hestate.file_name[0] = 0;

    // Set HEState variables
    g_hestate.scrolled = 0;

    // status bar & mode
    g_hestate.mode = MODE_NORMAL;
    g_hestate.status_message = buff_create();

    // Read command
    g_hestate.read_buff = buff_create();
    g_hestate.search_buff = buff_create();
    g_hestate.last_found.start = -1;
    g_hestate.last_found.end = -1;

    // Write
    g_hestate.last_byte = (byte_t){0};
    g_hestate.last_write_offset = -1;
    g_hestate.repeat_buff = buff_create();
    g_hestate.repeat = 1;

    g_hestate.action_list = action_list_init();
    g_hestate.in_ascii = false;

    g_hestate.grammars = grammar_list_create();

    g_hestate.selection.start = -1;
    g_hestate.selection.end   = -1;
    g_hestate.selection.is_backwards   = false;

    g_hestate.cursor_y = 1;
    g_hestate.cursor_x = 0;

    g_hestate.dirty = false;
    g_hestate.read_only = false;
    g_hestate.warned_read_only = false;

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
void editor_exit(void){
    if (g_hestate.read_buff != NULL){
        buff_remove(g_hestate.read_buff);
        g_hestate.read_buff = NULL;
    }
    if (g_hestate.search_buff != NULL){
        buff_remove(g_hestate.search_buff);
        g_hestate.search_buff = NULL;
    }
    if (g_hestate.repeat_buff != NULL){
        buff_remove(g_hestate.repeat_buff);
        g_hestate.repeat_buff = NULL;
    }
    if (g_hestate.buff != NULL){
        // Free the screen buff
        buff_remove(g_hestate.buff);
        g_hestate.buff = NULL;
    }
    if (g_hestate.status_message != NULL){
        // Free the status_mesasge
        buff_remove(g_hestate.status_message);
        g_hestate.status_message = NULL;
    }
    if (g_hestate.content != NULL){
        // Free the read file content
        free(g_hestate.content);
        g_hestate.content = NULL;
    }
    if (g_hestate.file_name != NULL){
        // Free the filename
        free(g_hestate.file_name);
        g_hestate.file_name = NULL;
    }
    if (g_hestate.status_message != NULL){
        // Free the status_mesasge
        buff_remove(g_hestate.status_message);
        g_hestate.status_message = NULL;
    }

    if (g_hestate.action_list != NULL){
        // Free the action list items, then free the action list itself
        action_cleanup(g_hestate.action_list);
        free(g_hestate.action_list);
        g_hestate.action_list = NULL;
    }

    if (g_hestate.grammars != NULL){
        grammar_cleanup(g_hestate.grammars);
        g_hestate.grammars = NULL;
    }

    term_clear_screen();

    // Exit raw mode
    term_disable_raw(&g_hestate.term_original);
}
