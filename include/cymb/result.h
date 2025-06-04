#ifndef CYMB_RESULT_H
#define CYMB_RESULT_H

/*
 * A result returned by most Cymb functions.
 */
typedef enum CymbResult
{
	CYMB_SUCCESS,
	CYMB_ERROR_UNKNOWN,
	CYMB_ERROR_FILE_NOT_FOUND,
	CYMB_ERROR_OUT_OF_MEMORY,
	CYMB_ERROR_INVALID_ARGUMENT
} CymbResult;

#endif
