#ifndef CYMB_ARGUMENTS_H
#define CYMB_ARGUMENTS_H

#include <stddef.h>

#include "cymb/result.h"

/*
 * A C standard version.
 */
typedef enum CymbVersion: long
{
	CYMB_C90,
	CYMB_C95 = 199409L,
	CYMB_C99 = 199901L,
	CYMB_C11 = 201112L,
	CYMB_C17 = 201710L,
	CYMB_C23 = 202311L
} CymbVersion;

/*
 * The compiling options.
 *
 * Fields:
 * - input: The path to the file to compile.
 * - output: The path to write the result to.
 * - version: The version of the C standard to use.
 * - tabWidth: Tab width used for diagnostics.
 * - debug: Switch to compile in debug or release mode.
 */
typedef struct CymbOptions
{
	char* input;
	char* output;

	CymbVersion version;

	unsigned char tabWidth;

	bool debug;
} CymbOptions;

/*
 * Parse arguments.
 *
 * Parameters:
 * - argumentCount: The number of arguments.
 * - arguments: The arguments.
 * - options: A pointer to the object storing compiling options.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_INVALID_ARGUMENT if an invalid argument was passed.
 * - CYMB_ERROR_OUT_OF_MEMORY if file paths could not be allocated.
 */
CymbResult cymbParseArguments(size_t argumentCount, const char* const* arguments, CymbOptions* options);

#endif
