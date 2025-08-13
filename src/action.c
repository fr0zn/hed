#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include <hed_action.h>
#include <hed_utils.h>

void action_add(HEActionList *list, enum action_type type, unsigned int offset,
    HEDByte byte, int repeat){


    HEAction *action = malloc(sizeof(HEAction));
    if (action == NULL) {
        perror("failed to allocate memory for action: ");
        exit(1);
    }
    action->type   = type;
    action->offset = offset;
    action->b      = byte;
    action->next   = NULL; // We are the last
    action->prev   = NULL; // In case we are the first
    action->repeat = repeat;

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

}

HEActionList* action_list_init(void){
    HEActionList * a_list = malloc(sizeof(HEActionList));
    if (a_list == NULL) {
        perror("failed to allocate memory for action list: ");
        exit(1);
    }
    a_list->first   = NULL;
    a_list->last    = NULL;
    a_list->current = NULL;

    action_add(a_list, ACTION_BASE, 0, (HEDByte){{0},{0},false, 0}, 0); // Add one base action

    return a_list;
}

void action_cleanup(HEActionList *list) {
    if(list == NULL || list->last == NULL) {
        return;
    }
    HEAction *prev;
    HEAction *action = list->last;

    do {
        prev = action->prev;
        free(action);
        action = prev;
    } while (action != NULL);
}
