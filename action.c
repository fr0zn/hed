#include "action.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>


void action_list_print(HEActionList* list) {
	HEAction* node = list->first;
	if (node == NULL) {
		fprintf(stderr, "Nothing to delete, head is null\n");
		return;
	}
	while (node != NULL) {
		fprintf(stderr, "(%d, %s, %02x-%02x) -> ", node->offset, action_names[node->type], node->b.o, node->b.c);
		node = node->next;
		if (node == NULL) {
			fprintf(stderr, "END\n");
		}
	}
}

void action_add(HEActionList *list, enum action_type type, unsigned int offset, unsigned char o, unsigned char c){

    HEAction *action = malloc(sizeof(HEAction));
    action->type   = type;
    action->offset = offset;
    action->b      = (byte_t){o,c};
    action->next   = NULL; // We are the last
    action->prev   = NULL; // In case we are the first

    // We are adding a new action after undo, so remove all actions
    // from the last one until we hit the current
    if(list->current != list->last && list->last != NULL){

        HEAction *tmp;

        while(list->current != list->last){
            tmp = list->last;
            list->last = list->last->prev;
            free(tmp);
        }
    }


    // First action
    if(list->first == NULL){
        list->first = action;
    }else{
        // The new action previous is the last in the list, and the next from previous is the new
        action->prev = list->last;
        list->last->next = action;
    }

    // The new action is now the current and last from the list
    list->last    = action;
    list->current = action;

    //action_list_print(list);

}

HEActionList* action_list_init(){
    HEActionList * a_list = malloc(sizeof(HEActionList));
    a_list->first   = NULL;
    a_list->last    = NULL;
    a_list->current = NULL;
    action_add(a_list, ACTION_BASE, 0, 0, 0); // Add one base action
    return a_list;
}
