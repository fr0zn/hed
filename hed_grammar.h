#ifndef HE_GRAMMAR_H
#define HE_GRAMMAR_H

#include <stdbool.h>
#include <stdint.h>

#define GRAMMAR_DEFAULT_SIZE 12

enum color_bg{
    COLOR_NOCOLOR = 0,
    COLOR_RED_BG = 41,
    COLOR_GREEN_BG,
    COLOR_YELLOW_BG,
    COLOR_BLUE_BG,
    COLOR_MAGENTA_BG,
    COLOR_CYAN_BG,
    COLOR_LGRAY_BG,
    COLOR_DGRAY_BG = 100
};

typedef struct grammar_t {
    char *name;
    int start;
    int end;
    int id;
    enum color_bg color;
}HEGrammar;

typedef struct {
    HEGrammar *content; // List of grammars
    int len;
    int capacity;

    int *free_ids;
    int free_len;
    int free_capacity;
}HEGrammarList;

enum grammar_step {
    GRAMMAR_NAME,
    GRAMMAR_COLOR
};


HEGrammarList* grammar_list_create();

void grammar_print_list(HEGrammarList* list);

int grammar_add(HEGrammarList* list, char *name, int start, int end, enum color_bg color);

enum color_bg grammar_color_id(HEGrammarList* list, int id);

HEGrammar* grammar_id(HEGrammarList* list, int id);

bool grammar_del(HEGrammarList* list, int id);


#endif
