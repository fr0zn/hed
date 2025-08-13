#include <hed_search.h>
#include <hed_buff.h>

int search_buffer(HEDByte* buffer, int buff_len, HEDBuff* search_pattern, 
	int start_offset, SEARCH_DIRECTION direction) {

	int offset = -1;

	if (direction == SEARCH_FORWARD) {
	 	for (int b = start_offset; b < buff_len; b++) {
	 		for (unsigned int p = 0; p < search_pattern->len; p++) {
	 			if ((search_pattern->content[p] & 0xff) == (buffer[b+p].c.value & 0xff)) {
	 				// If we found the first byte match
	 				if (p == 0) offset = b;
	 				// Is a full match
	 				if (p == search_pattern->len - 1) return offset;
	 				continue;
	 			}
	 			// If we get here, one of the bytes of buffer is not the same as
	 			// the search_pattern
	 			offset = -1;
	 			break;
			}
	 	}
	} else if (direction == SEARCH_BACKWARD) {
		for (int b = start_offset; b >= 0; b--) {
	 		for (unsigned int p = 0; p < search_pattern->len; p++) {
	 			if ((search_pattern->content[p] & 0xff) == (buffer[b+p].c.value & 0xff)) {
	 				// If we found the first byte match
	 				if (p == 0) offset = b;
	 				// Is a full match
	 				if (p == search_pattern->len - 1) return offset;
	 				continue;
	 			}
	 			// If we get here, one of the bytes of buffer is not the same as
	 			// the search_pattern
	 			offset = -1;
	 			break;
			}
	 	}
	} 

	return SEARCH_NOT_FOUND;
}
