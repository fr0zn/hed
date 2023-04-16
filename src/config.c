#include <hed_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>

#include <hed_term.h>
#include <hed_buff.h>
#include <hed_editor.h>

HEDConfig* config_create_default() {
    return config_create(2,8,0,1);
}

HEDConfig* config_create(int group, int groups_per_line, int insert_nibble, int replace_nibble){
    HEDConfig* conf = malloc(sizeof(HEDConfig));
    conf->bytes_group = group;
    conf->groups_per_line = groups_per_line;
    conf->insert_nibble = insert_nibble;
    conf->replace_nibble = replace_nibble;
    return conf;
}

char* config_open(char* filename){

    char *buff;

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return NULL;
    }

    struct stat statbuf;
    if(stat(filename, &statbuf) == -1) {
        return NULL;
    }

    if (!S_ISREG(statbuf.st_mode)) {
        fprintf(stderr, "File '%s' is not a regular file\n", filename);
        return NULL;
    }

    if (statbuf.st_size == 0) {
        return NULL;
    }

    buff = malloc(statbuf.st_size);

    fread(buff, statbuf.st_size, 1, fp);
    fclose(fp);

    return buff;
}

int numbers_only(const char *s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return 0;
    }

    return 1;
}

int config_update_key_value(HEDConfig* config, HEDBuff* key, HEDBuff* value) {

    char *k = key->content;
    char *v = value->content;

    int error = 0;

    if (strcmp(k, "bytes") == 0) {

        if (numbers_only(v)){
            int value_int = atoi(v);
            if( value_int == 0 ){
                error = 2;
            } else {
                config->bytes_group = value_int;
                return 0;
            }
        }
        error = 2;

    } else if (strcmp(k, "groups") == 0) {

        if (numbers_only(v)){
            int value_int = atoi(v);
            if( value_int == 0 ){
                error = 2;
            } else {
                config->groups_per_line = value_int;
                return 0;
            }
        }

        error = 2;

    } else if (strcmp(k, "insert") == 0) {

        if (numbers_only(v)){
            int value_int = atoi(v);
            if ( (value_int != 0) && (value_int != 1) ){
                error = 2;
            } else {
                config->insert_nibble = value_int;
                return 0;
            }
        }

        error = 2;
    } else if (strcmp(k, "replace") == 0) {

        if (numbers_only(v)){
            int value_int = atoi(v);
            if( (value_int != 0) && (value_int != 1)){
                error = 2;
            } else {
                config->replace_nibble = value_int;
                return 0;
            }
        }

        error = 2;
    } else {
        // NO valid key
        error = 1;
    }

    term_set_format(FG_RED);

    if (error == 2) {
        printf("Invalid value '%s' for option '%s'\r\n", v, k);
    }

    if (error == 1) {
        printf("Invalid option '%s'\r\n", k);
    }

    term_set_format(FORMAT_RESET);

    fflush(stdout);

    return error;
}


void config_parse_line_key_value(HEDBuff* original_line,
    HEDBuff* key, HEDBuff* value) {

    if (original_line->len <= 0) {
        return;
    }

    char *token;
    char *line = malloc(original_line->len+1);
    char *to_free = line;

    strncpy(line, original_line->content, original_line->len);
    line[original_line->len] = 0;

    token = strtok(line, " =");
    if (strcmp(token, "set") == 0) {
        token = strtok(NULL, " =");
        if (token != NULL) {
            buff_append(key, token, strlen(token));
        }
        token = strtok(NULL, " =");
        if (token != NULL) {
            buff_append(value, token, strlen(token));
        }
    }
    free(to_free);
}

int config_parse(HEDConfig* config, char* buff){
    char c;
    bool in_comment = false;
    HEDBuff* line_buff = buff_create();
    HEDBuff* key = buff_create();
    HEDBuff* value = buff_create();
    int line = 0;
    int ret = 0;

    while ((c = *buff++)) {
        if(c == EOF) break;
        if (c == '\n'){
            in_comment = false;
            config_parse_line_key_value(line_buff, key, value);
            if (key->len > 0){
                ret = config_update_key_value(config, key, value);
            }
            if (ret != 0){
                return line;
            }
            buff_clear_dirty(line_buff);
            buff_clear_dirty(key);
            buff_clear_dirty(value);
            line++;
        }else {
            if (in_comment) {
            continue;
            }
            if (c == '#') {
                in_comment = true;
                continue;
            }
            buff_append(line_buff, &c, 1);
        }

    }

    buff_remove(line_buff);
    buff_remove(key);
    buff_remove(value);

    return -1;
}
