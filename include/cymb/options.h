#ifndef CYMB_ARGUMENTS_H
#define CYMB_ARGUMENTS_H

#include "cymb/diagnostic.h"
#include "cymb/result.h"

/*
 * A C standard version.
 */
typedef enum CymbStandard: long
{
	CYMB_C90,
	CYMB_C95 = 199409L,
	CYMB_C99 = 199901L,
	CYMB_C11 = 201112L,
	CYMB_C17 = 201710L,
	CYMB_C23 = 202311L
} CymbStandard;

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
	const char** inputs;
	size_t inputCount;
	const char* output;

	CymbStandard standard;

	unsigned char tabWidth;

	bool debug: 1;
	bool version: 1;
	bool help: 1;
} CymbOptions;

/*
 * Parse arguments.
 *
 * Parameters:
 * - arguments: The arguments.
 * - argumentCount: The number of arguments.
 * - options: A pointer to the object storing the options.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_INVALID_ARGUMENT if an invalid argument was passed.
 * - CYMB_ERROR_OUT_OF_MEMORY if out of memory.
 */
CymbResult cymbParseArguments(const CymbConstString* arguments, size_t argumentCount, CymbOptions* options, CymbDiagnosticList* diagnostics);

#endif
