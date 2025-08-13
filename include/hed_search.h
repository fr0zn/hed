#ifndef HED_SEARCH_H
#define HED_SEARCH_H

#include <hed_buff.h>

typedef struct {
	int start;
	int end;
} HEDFound;

typedef enum {

	SEARCH_FORWARD,
	SEARCH_BACKWARD,

}SEARCH_DIRECTION;

typedef enum {

	SEARCH_NOT_FOUND = -1

} SEARCH_RESULT;

/**
 * @brief      Searches for a given pattern in a buffer with a direction
 *
 * @return     Returns the found offset or top/bottom limit
 */
int search_buffer(HEDByte* buffer, int buff_len, HEDBuff* search_pattern, 
	int start_offset, SEARCH_DIRECTION direction);

#endif
