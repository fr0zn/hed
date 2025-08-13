#include <stdio.h>
#include <unistd.h>

#include <hed_editor.h>

#ifndef HED_VERSION
#define HED_VERSION "1.0.0 (no git)"
#endif

#ifndef HED_VERSION_SHORT
#define HED_VERSION_SHORT "1.0.0"
#endif

#ifndef HED_GIT_HASH
#define HED_GIT_HASH "unknown"
#endif

static void print_version(void) {
	printf("hed %s\ncommit: %s\n", HED_VERSION, HED_GIT_HASH);
}

static void print_help(void) {
	fprintf(stderr,
	"usage: hed [-hv] filename\n"\
	"\n"
	"Command options:\n"
	"    -h     Print this message and exits\n"
	"    -v     Version information\n"
	"\n"
	"Bugs: see <http://github.com/fr0zn/hed>\n");
}

int main(int argc, char *argv[]){


    int ch = 0;
	while ((ch = getopt(argc, argv, "vh")) != -1) {
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

    editor_init();

    if(argc > 1){
        editor_open_file(argv[1]);
    }

    while(1){
        editor_refresh_screen();
        editor_process_keypress();
    }
    return 0;
}

