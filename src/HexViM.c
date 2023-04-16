#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <hed_editor.h>
#include <hed_config.h>

#ifndef HED_VERSION
#define HED_VERSION "1.0.1 (no git)"
#endif

#ifndef HED_VERSION_SHORT
#define HED_VERSION_SHORT "1.0.1"
#endif

#ifndef HED_GIT_HASH
#define HED_GIT_HASH "unknown"
#endif

void setArgToConfig(char arg[]);

static void print_version() {
    printf("hed %s\ncommit: %s\n", HED_VERSION, HED_GIT_HASH);
}

static void print_help() {
    fprintf(stderr,
    "usage: hed [-hv] g=N G=N i=N r=N filename\n"\
    "\n"
    "Command options:\n"
    "    -h     Print this message and exits\n"
    "    -v     Version information\n"
	"    g=2    Set bytes group\n"
    "    G=8    Set groups per line\n"
    "    i=0    Set insert nibble\n"
    "    r=1    Set replace nibble\n"
    "\n"
    "Bugs: see <http://github.com/fr0zn/hed>\n");
}

HEDConfig *config;
char opt[]  = "vh";
int fileNameIndex = -1;
int confBytesGroup = 2;
int confGroupPerLine = 8;
int confInsertNibble = 0;
int confReplaceNibble = 1;

int main(int argc, char *argv[]){
    int ch = 0;
    while ((ch = getopt(argc, argv, opt)) != -1) {
        switch (ch) {
        case 'v':
            print_version();
            exit(0);
            break;
        case 'h':
            print_help();
            exit(0);
            break;
        default:
            print_help();
            exit(1);
            break;
        }
    }

    int i = 1;
    char *cha = argv[i];
    while(cha != NULL){
        if (strstr(argv[i], "=") != NULL) {
            printf("%s\n", argv[i]);
            setArgToConfig(argv[i]);
        } else {
            printf("file name : %s\n", argv[i]);
            fileNameIndex = i;
        }
        i++;
        cha = argv[i];
    }

    config = config_create(confBytesGroup, confGroupPerLine, confInsertNibble, confReplaceNibble);

    editor_init(config);

    if(fileNameIndex != -1){
        /* src/editor.c */
        editor_open_file(argv[fileNameIndex]);
    }

    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

void setArgToConfig(char arg[]){
    char mod;
    int value;
    bool isArgSet = true;
    char *token = strtok(arg, "=");
    while (token != NULL){
        if (isArgSet){
            mod = *token;
            isArgSet = false;
        } else {
            value = *token - 48;
        }
        token = strtok(NULL, "=");
    }
    printf("mod : %d\nvalue : %d\n", mod, value);
    switch(mod){
        //'g'
        case 103:
            confBytesGroup = value;
            break;
        // 'G'
        case 71:
            confGroupPerLine = value;
            break;
        // 'i'
        case 105:
            confInsertNibble = value;
            break;
        // 'r'
        case 114:
            confReplaceNibble = value;
            break;
        default:
            fprintf(stderr, "%c(%d) is undefited mode", mod, mod);
            exit(1);
    }
}

