#ifndef HED_ACTION_H
#define HED_ACTION_H

#include <hed_utils.h>
#include <hed_types.h>

enum action_type {
    ACTION_BASE, // Used as the base of actions
    ACTION_REPLACE,
    ACTION_INSERT,
    ACTION_DELETE,
    ACTION_APPEND
};

typedef struct action_t{
    struct action_t *prev;
    struct action_t *next;

    enum action_type type;
    unsigned int offset;

    HEDByte b; // Changed/modified byte (original)

    int repeat;

}HEAction;

typedef struct {
    HEAction *first;
    HEAction *last;
    HEAction *current; // So we can redo

}HEActionList;

HEActionList* action_list_init();
void action_add(HEActionList *list, enum action_type type,
    unsigned int offset, HEDByte byte, int repeat);

void action_cleanup(HEActionList *list);
void __debug__print_action_list(HEActionList *list);

#endif
