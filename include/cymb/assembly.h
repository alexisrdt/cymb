#ifndef CYMB_ASSEMBLY_H
#define CYMB_ASSEMBLY_H

#include <stdint.h>

#include "cymb/diagnostic.h"

/*
 * Assemble assembly code to codes.
 *
 * Parameters:
 * - string: The assembly code to assemble.
 * - codes: The resulting codes.
 * - count: The resulting number of codes.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if some allocation failed.
 */
CymbResult cymbAssemble(const char* string, uint32_t** codes, size_t* count, CymbDiagnosticList* diagnostics);

/*
 * Disassemble codes to assembly code.
 *
 * Parameters:
 * - codes: The codes to disassemble.
 * - count: The number of codes.
 * - string: The resulting assembly code.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
CymbResult cymbDisassemble(const uint32_t* codes, size_t count, CymbString* string, CymbDiagnosticList* diagnostics);

#endif
