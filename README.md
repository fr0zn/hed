# hed

vim like hex editor

<img src="https://i.imgur.com/MyVMJP5.png" width="600">

# Compiling and running

The project does not have a dependency or libraries.

To compile and install:

```
git clone https://github.com/fr0zn/hed
cd hed
make
sudo make install
```

The Makefile will install the binary `hed` under /usr/bin/hed and a manpage
under `/usr/local/share/man/man1/hed.1`.


Running `hed`:
```
hed                # open an empty file
hed filename       # open a file
hed -h             # for help
hed -v             # version information
```
# Key bindings

All commands can be prefixed with a number 'n' to repeat it 'n' times.

### Navigate and move
```
h / left   : Move cursor one byte left in the buffer.
l / right  : Move cursor one byte right in the buffer.
j / down   : Move cursor one position up.
k / up     : Move cursor one position down.
w          : Move cursor one group of bytes right.
b          : Move cursor one group of bytes left.
gg         : Go to the beginning of the file.
G          : Go to the end of the file. If 'n' number is typed before,
             go to offset 'n' instead.
0          : Go to the beginning of the line.
$          : Go to the end of the line.
TAB        : Switch cursor position between ascii and hex.
```
### Search
```
/          : Start search input forward. Can be used to search for
             byte value "\deedbeef" or strings "\ELF" depending on
             the cursor side.
?          : Same as before but searches backwards.
n          : Search for next occurrence (forward).
N          : Search for previous occurrence (backward).
```
### Add, remove and insert bytes
```
i          : Enable INSERT mode.
r          : Enable REPLACE mode.
v          : Enable VISUAL mode.
a          : Enable APPEND mode. If 'A' is used instead, moves to the end
             of the line first.
]          : Increment the byte at the cursor position. If 'n' is given,
             increments 'n' times. (modes: NORMAL)
[          : Decrement the byte at the cursor position. If 'n' is given,
             decrements 'n' times. (modes: NORMAL)
x / DEL    : Delete byte at current cursor position. If 'n' is given,
             delete 'n' bytes. (modes: NORMAL, VISUAL)
dot (.)    : Repeats the last write. (modes: NORMAL)
```
### Undo, redo and commands
```
:          : Enable COMMAND mode.
ESC        : Enable NORMAL mode.
u          : Undo the last action. If 'n' is given, undoes 'n' times.
CTRL+R     : Redo the last action. If 'n' is given, redoes 'n' times.
```
# Command mode

In command-mode, manual commands can be entered. This can be triggered by 
typing the colon ( : ) character. Current implemented commands are:
```
NUM               Go to offset in base 10.
o[ffset] NUM      Same as before.

0xHEXVALUE        Go to offset in base 16.
o[ffset] 0xHEX    Same as before.

set [OPTION]=val  See section OPTIONS

w[rite] [file]    Write current buffer to disk. If file is given, write
                  the buffer in the new file name.
e[dit] [file]     Open a new file to edit, will remind to save previous
                  if its edited.
q                 quit (add ! to force quit)
```

# Configuration file

`~/.hedrc` Can contain some initialization settings that `hed` will read. The default configuration file is: 

```
# Number of bytes per group
set bytes = 2

# Number of groups per line
set groups = 8

# Nibble to write first
# value: 0
#              v
# 00000000: | cf
# value: 1
#             v
# 00000000: | cf

# Nibble to write first on insert mode
set insert = 0

# Nibble to replace first on replace mode
set replace = 1
```

This can be copied to `~/.hedrc` and modified as you like, all lines starting with '#' are ignored.

# Examples

```
3j                Move 3 lines down
3w                Move 3 bytes group to the right
10x               Remove the following 10 bytes
10r90             Replace 10 bytes with value 0x90
<TAB>10r9         Replace 10 bytes with '9' (ascii)
10i90             Inserts 10 bytes with value 0x90
4i9<ESC>          Inserts 4 values 0x9, (0x99 0x99)
0V$               Select the entire line (moving to the beginning first)
0V$r0<ESC>        Select the entire line and replace it with 0's
ggVG              Select the entire file
:w out.bin        Writes the current buffer to file 'out.bin'
:q!               Quits without saving
:set bytes=4      Sets the number of bytes per group
```

## Authors

Implemented by Ferran Celades <fr0zn@protonmail.com>, with indirect help of Kevin Pors <https://github.com/krpors/hx>
