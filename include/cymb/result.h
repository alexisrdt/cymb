#ifndef CYMB_RESULT_H
#define CYMB_RESULT_H

/*
 * A result returned by most Cymb functions.
 */
typedef enum CymbResult
{
	CYMB_SUCCESS,
	CYMB_NO_MATCH,
	CYMB_INVALID,
	CYMB_FILE_NOT_FOUND,
	CYMB_OUT_OF_MEMORY
} CymbResult;

#endif
