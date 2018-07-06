#ifndef HED_CONFIG_H
#define HED_CONFIG_H

typedef struct {
    int bytes_group;
    int groups_per_line;
} HEDConfig;

#include <hed_buff.h>

HEDConfig* config_create_default();

char* config_open(char* filename);

int numbers_only(const char *s);

int config_update_key_value(HEDConfig* config,
    HEDBuff* key, HEDBuff* value);

void config_parse_line_key_value(HEDBuff* original_line,
                HEDBuff* key,
                HEDBuff* value);

int config_parse(HEDConfig* config, char* buff);


#endif
