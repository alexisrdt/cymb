#ifndef CYMB_ASSEMBLY_H
#define CYMB_ASSEMBLY_H

#include <stdint.h>

#include "cymb/diagnostic.h"
#include "cymb/options.h"

/*
 * Assemble assembly code to codes.
 *
 * Parameters:
 * - string: The assembly code to assemble.
 * - codes: The resulting codes.
 * - size: The resulting size of codes.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if some allocation failed.
 */
CymbResult cymbAssemble(const char* string, char** codes, size_t* size, CymbMachine machine, CymbDiagnosticList* diagnostics);

#endif
