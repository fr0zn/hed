#ifndef HE_ACTION_H
#define HE_ACTION_H

#include "utils.h"

enum action_type {
    ACTION_BASE, // Used as the base of actions
    ACTION_REPLACE,
    ACTION_INSERT,
    ACTION_DELETE,
    ACTION_APPEND
};

static const char* action_names[] = {
    "base",
    "replace",
    "insert",
    "delete",
    "append"
};

typedef struct action_t{
    struct action_t *prev;
    struct action_t *next;

    enum action_type type;
    unsigned int offset;

    byte_t b; // Changed/modified byte (original)

}HEAction;

typedef struct {
    HEAction *first;
    HEAction *last;
    HEAction *current; // So we can redo

}HEActionList;


HEActionList* action_list_init();
void action_add(HEActionList *list, enum action_type type, unsigned int offset, unsigned char o, unsigned char c);
void __debug__print_action_list(HEActionList *list);

#endif
