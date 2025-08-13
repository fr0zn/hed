#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <hed_term.h>
#include <hed_buff.h>


void term_get_size(unsigned int* rows, unsigned int* cols){

    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0){
        perror("Failed to query terminal size: ");
        exit(1);
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;
}

void term_print(const char *data, ssize_t len) {
    // Write to stdout by default
    if (write(STDOUT_FILENO, data, len) == -1) {
        perror("Couldn't write the content: ");
        exit(1);
    }
}

int term_set_format(int format_color) {
    char buff[16]; // Max int value is 10 digits. 10 + 3 characters for
                   // the escape sequence. With a buffer of 16 its enought
    int w = sprintf(buff, "\x1b[%dm", format_color);
    term_print(buff, w);
    return w;
}

int term_set_format_buff(HEDBuff* buff, int format_color) {
    return buff_vappendf(buff, "\x1b[%dm", format_color);
}

void term_clear_line_buff(HEDBuff* buff) {
    buff_append(buff, TERM_CLEAR_LINE, TERM_CLEAR_LINE_LEN);
}

void term_clear_line_end(void) {
    term_print(TERM_CLEAR_LINE_END, TERM_CLEAR_LINE_END_LEN);
}

void term_clear_line_end_buff(HEDBuff* buff) {
    buff_append(buff, TERM_CLEAR_LINE_END, TERM_CLEAR_LINE_END_LEN);
}

void term_cursor_hide(void){
    term_print(TERM_CURSOR_HIDE, TERM_CURSOR_HIDE_LEN);
}

void term_cursor_show(void){
    term_print(TERM_CURSOR_SHOW, TERM_CURSOR_SHOW_LEN);
}

void term_goto(unsigned int row, unsigned int col) {
    char buff[32]; // Max int value is 10 digits. 2*10 + 3 characters for
                   // the escape sequence. With a buffer of 32 its enought
    int w = sprintf(buff, "\x1b[%u;%uH", row, col);
    term_print(buff, w);
}

void term_goto_buff(HEDBuff* buff, unsigned int row, unsigned int col) {
    // Using buffer valist append format
    buff_vappendf(buff, "\x1b[%u;%uH", row, col);
}

void term_enable_raw(struct termios* term_original) {

    // Checks if the current terminal is a TTY
    if (!isatty(STDIN_FILENO)) {
        perror("Input is not a TTY: ");
        exit(1);
    }

    // Gets all the attributes of the terminal storing them on `term_original`
    tcgetattr(STDIN_FILENO, term_original);

    // Creates a copy of the original terminal that will be used as a base
    // for the raw terminal
    struct termios raw = *term_original;

    // Check: TERMIOS(3) `cfmakeraw` function for reference
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Non canonical read
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    // Sets all the new raw attributes to the terminal
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        perror("Unable to set terminal to raw mode: ");
        exit(1);
    }
}

void term_disable_raw(struct termios* term_original) {
    // Reset the state of the terminal to the term_original information
    tcsetattr(STDIN_FILENO, TCSAFLUSH, (term_original));
    // Show the terminal cursor again
    term_print("\x1b[?25h", 6);
}

void term_clear_screen(void){
    // Reset color \x1b[0m
    // Move top-left line \x1b[H
    // Clear until the bottom of the screen \x1b[2J
    term_print("\x1b[0m\x1b[H\x1b[2J", 11);
}
