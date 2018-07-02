##
## Helper used to obtain the values in the KEY_CODE enum `include/hed_types.h`
## The parsing is done in the `read_key` function at `include/hed_read.h`
##

import sys
import tty
import termios
import string

def print_char(ch):
    if ch == chr(0x1b):
        sys.stdout.write('\r'+ch.encode('hex')+' (ESC)'+'\n')
    elif ch in string.printable:
        sys.stdout.write('\r'+ch.encode('hex')+' ('+ch+')'+'\n')
    else:
        sys.stdout.write('\r'+ch.encode('hex')+'\n')

sys.stdout.write("Type 'q' to exit\r\n")

fd = sys.stdin.fileno()
old_settings = termios.tcgetattr(fd)
tty.setraw(sys.stdin)

ch = None
while(ch != 'q'):
    ch = sys.stdin.read(1)
    print_char(ch)

termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
