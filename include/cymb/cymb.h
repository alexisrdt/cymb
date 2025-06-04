#ifndef CYMB_CYMB_H
#define CYMB_CYMB_H

#include <stdio.h>

#include "cymb/arguments.h"
#include "cymb/diagnostic.h"
#include "cymb/lex.h"
#include "cymb/memory.h"
#include "cymb/result.h"
#include "cymb/tree.h"

/*
 * Read all the contents of a file in text mode.
 *
 * Parameters:
 * - path: Path of the file.
 * - string: A pointer to a string where to store the contents of the file.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_ERROR_OUT_OF_MEMORY if the file is too large.
 * - CYMB_ERROR_UNKNOWN otherwise.
 */
CymbResult cymbReadFile(const char* path, CymbString* string);

/*
 * Compile a source file.
 *
 * Parameters:
 * - options: A pointer to the options to use for compilation.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_ERROR_OUT_OF_MEMORY if the code is too large.
 * - CYMB_ERROR_UNKNOWN otherwise.
 */
CymbResult cymbCompile(const CymbOptions* options);

#endif
