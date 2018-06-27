#include "term.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <termios.h>

// Gets the current terminal size and stores the value to the HEState struct
void term_get_size(HEState* state){
    struct winsize ws;
    if ( ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 ){
        perror("Failed to query terminal size");
        exit(1);
    }

    state->screen_rows = ws.ws_row;
    state->screen_cols = ws.ws_col;
}

// Enables terminal raw mode
void term_enable_raw(HEState* state){
    if(!isatty(STDIN_FILENO)){
        perror("Input is not a TTY");
        exit(1);
    }

    tcgetattr(STDIN_FILENO, &(state->term_original));

    // Creates a copy of the original terminal
    struct termios raw = state->term_original;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0){
        perror("Unable to set terminal to raw mode");
        exit(1);
    }
}

// Disable terminal raw mode
void term_disable_raw(HEState* state){
    // Reset the state of the terminal before the program
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &(state->term_original));
    // Show the cursor again
    write(STDOUT_FILENO, "\x1b[?25h]", 6);

}

// Clear the screen and colors
void term_clear(){
    // Clear the color, move the cursor up-left, clear the screen
    write(STDOUT_FILENO, "\x1b[0m\x1b\[H\x1b[2J", 11);
}

// Draws a buffer on the terminal
void term_draw(HEBuff* buff){

    if(write(STDOUT_FILENO, buff->content, buff->len) == -1){
        perror("Couldn't write the HEBuff content");
        exit(1);
    }
}
