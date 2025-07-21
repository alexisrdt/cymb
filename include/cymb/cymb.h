#ifndef CYMB_CYMB_H
#define CYMB_CYMB_H

#include "cymb/diagnostic.h"
#include "cymb/lex.h"
#include "cymb/memory.h"
#include "cymb/options.h"
#include "cymb/reader.h"
#include "cymb/result.h"
#include "cymb/tree.h"
#include "cymb/version.h"

/*
 * Run Cymb.
 *
 * Parameters:
 * - arguments: Arguments to configure Cymb.
 * - argumentCount: The number of arguments.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if out of memory.
 * - CYMB_ERROR_INVALID_ARGUMENT if an argument is invalid or a file could not be compiled.
 */
CymbResult cymbMain(const CymbConstString* arguments, size_t argumentCount);

#endif
