#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hed_grammar.h>

#define GRAMMAR_LIST_DEFAULT_SIZE 32

HEGrammarList* grammar_list_create(void){
    HEGrammarList *list = malloc(sizeof(HEGrammarList));
    if (list == NULL) {
        perror("Failed to allocate grammar list");
        exit(1);
    }

    list->content = malloc(GRAMMAR_LIST_DEFAULT_SIZE * sizeof(HEGrammar));
    if (list->content == NULL) {
        perror("Failed to allocate grammar list content");
        exit(1);
    }

    list->len = 0;
    list->capacity = GRAMMAR_LIST_DEFAULT_SIZE;

    list->free_ids = malloc(GRAMMAR_LIST_DEFAULT_SIZE);
    list->free_len = 0;
    list->free_capacity = GRAMMAR_LIST_DEFAULT_SIZE;

    return list;
}


HEGrammar* grammar_id(HEGrammarList* list, int id){
    int i = 0;
    while(i < list->len && list->content[i].id != id){
        i++;
    }

    if(i == list->len){
        return NULL;
    }
    return &list->content[i];
}

enum color_bg grammar_color_id(HEGrammarList* list, int id){
    HEGrammar* gmr = grammar_id(list, id);
    if(gmr != NULL){
        return gmr->color;
    }
    return COLOR_NOCOLOR;
}
