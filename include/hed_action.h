#ifndef HED_ACTION_H
#define HED_ACTION_H

#include <hed_utils.h>
#include <hed_types.h>

HEActionList* action_list_init();
void action_add(HEActionList *list, enum action_type type, unsigned int offset, unsigned char o, unsigned char c);
void __debug__print_action_list(HEActionList *list);

#endif
