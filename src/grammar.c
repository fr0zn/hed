#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <hed_grammar.h>

#define GRAMMAR_LIST_DEFAULT_SIZE 32

HEGrammarList* grammar_list_create(){
    HEGrammarList *list = malloc(sizeof(HEGrammarList));

    list->content = malloc(GRAMMAR_LIST_DEFAULT_SIZE * sizeof(HEGrammar));

    list->len = 0;
    list->capacity = GRAMMAR_LIST_DEFAULT_SIZE;

    list->free_ids = malloc(GRAMMAR_LIST_DEFAULT_SIZE);
    list->free_len = 0;
    list->free_capacity = GRAMMAR_LIST_DEFAULT_SIZE;

    return list;
}

void grammar_print_list(HEGrammarList* list){
    for(int i = 0; i< list->len; i++){
        HEGrammar *g = &list->content[i];
        if(g->id != -1){
            printf("id: %d, name: %s, start: %d, end: %d, color: %d\n", g->id, g->name, g->start, g->end, g->color);
        }else{
            printf("(Deleted) %d\n", g->id);
        }
    }

    printf("Free list:\n");
    for(int i = 0; i < list->free_len; i++){
        printf("id: %d\n", list->free_ids[i]);
    }
}

int grammar_add(HEGrammarList* list, char *name, int start, int end, enum color_bg color){

    int id;

    if(list->free_len > 0){
        // Use the most recently deleted grammar
        id = list->free_ids[list->free_len-1];
        list->free_len--;
    }else{
        // No free grammar. Append one, and reallocated if needed
        id = list->len;
        if((list->len + 1) >= list->capacity){
            list->content = realloc(list->content, 2*list->capacity*sizeof(HEGrammar));
            list->capacity *= 2;
        }
        list->len++;
    }


    list->content[id].name  = malloc(strlen(name) + 1);
    strcpy(list->content[id].name, name);

    list->content[id].start = start;
    list->content[id].end   = end;
    list->content[id].color = color;
    list->content[id].id    = id;


    // Return grammar index (id)
    return id;
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

bool grammar_del(HEGrammarList* list, int id){

    HEGrammar* gmr = grammar_id(list, id);

    if(gmr != NULL){
        if((list->free_len + 1) >= list->free_capacity){
            list->free_ids = realloc(list->free_ids, 2*list->free_capacity*sizeof(int));
            list->free_capacity *= 2;
        }

        // Add the grammar index (id) to the free list
        list->free_ids[list->free_len] = gmr->id;
        list->free_len++;

        // Free the name of grammar and set index to not used
        if(gmr->name != NULL){
            free(gmr->name);
        }
        list->content[gmr->id].id = -1;
    }

    return false;
}
