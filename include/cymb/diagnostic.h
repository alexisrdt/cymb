#ifndef CYMB_DIAGNOSTIC_H
#define CYMB_DIAGNOSTIC_H

#include <stddef.h>

#include "cymb/memory.h"
#include "cymb/result.h"

/*
 * A diagnostic type.
 */
typedef enum CymbDiagnosticType
{
	// Options.
	CYMB_TOO_MANY_INPUTS,
	CYMB_UNKNOWN_OPTION,
	CYMB_MISSING_ARGUMENT,
	CYMB_UNEXPECTED_ARGUMENT,
	CYMB_INVALID_ARGUMENT,
	// Tokens.
	CYMB_UNKNOWN_TOKEN,
	CYMB_INVALID_CONSTANT_SUFFIX,
	CYMB_INVALID_CHARACTER_CONSTANT,
	CYMB_INVALID_STRING_CHARACTER,
	CYMB_UNFINISHED_STRING,
	CYMB_CONSTANT_TOO_LARGE,
	CYMB_SEPARATOR_AFTER_BASE,
	CYMB_DUPLICATE_SEPARATOR,
	CYMB_TRAILING_SEPARATOR,
	// Nodes.
	CYMB_UNEXPECTED_TOKEN,
	CYMB_UNMATCHED_PARENTHESIS,
	CYMB_MULTIPLE_CONST,
	CYMB_MULTIPLE_RESTRICT,
	CYMB_MULTIPLE_STATIC
} CymbDiagnosticType;

/*
 * A position in a file.
 *
 * Fields:
 * - line: The line.
 * - column: The column.
 */
typedef struct CymbPosition
{
	size_t line;
	size_t column;
} CymbPosition;

/*
 * Diagnostic information.
 *
 * Fields:
 * - position: The file position.
 * - line: The line.
 * - hint: The relevant part of the line.
 */
typedef struct CymbDiagnosticInfo
{
	CymbPosition position;

	CymbStringView line;
	CymbStringView hint;
} CymbDiagnosticInfo;

/*
 * A diagnostic.
 *
 * Fields:
 * - type: The diagnostic type.
 * - info: The diagnostic info.
 */
typedef struct CymbDiagnostic
{
	CymbDiagnosticType type;

	CymbDiagnosticInfo info;
} CymbDiagnostic;

/*
 * A list of diagnostics.
 *
 * Fields:
 * - file: The file for which diagnostics are emitted.
 * - tabWidth: The tab width used for diagnostics.
 * - diagnostics: The diagnostics.
 * - count: The number of diagnostics.
 * - capacity: The capacity of the diagnostics array.
 */
typedef struct CymbDiagnosticList
{
	const char* file;

	unsigned char tabWidth;

	CymbDiagnostic* diagnostics;
	size_t count;
	size_t capacity;
} CymbDiagnosticList;

/*
 * A list of constant diagnostics.
 *
 * Fields:
 * - file: The file for which diagnostics are emitted.
 * - tabWidth: The tab width used for diagnostics.
 * - diagnostics: The diagnostics.
 * - count: The number of diagnostics.
 */
typedef struct CymbConstDiagnosticList
{
	const char* file;

	unsigned char tabWidth;

	const CymbDiagnostic* diagnostics;
	size_t count;
} CymbConstDiagnosticList;

/*
 * Print diagnostics.
 *
 * Parameters:
 * - diagnostics: The diagnostics to print.
 */
void cymbDiagnosticListPrint(const CymbConstDiagnosticList* diagnostics);

/*
 * Create a diagnostic list.
 *
 * Parameters:
 * - diagnostics: A pointer to the list to create.
 * - file: The file for which diagnostics are emitted.
 * - tabWidth: The tab width used for diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if the list could not be created.
 */
CymbResult cymbDiagnosticListCreate(CymbDiagnosticList* diagnostics, const char* file, unsigned char tabWidth);

/*
 * Free a list of diagnostics.
 *
 * Parameters:
 * - diagnostics: The list to free.
 */
void cymbDiagnosticListFree(CymbDiagnosticList* diagnostics);

/*
 * Add a diagnostic to a list.
 *
 * Parameters:
 * - diagnostics: The list to modify.
 * - diagnostic: The diagnostic to add.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if the diagnostic could not be added.
 */
CymbResult cymbDiagnosticAdd(CymbDiagnosticList* diagnostics, const CymbDiagnostic* diagnostic);

/*
 * Get the next tab stop column.
 *
 * Parameters:
 * - column: The current column.
 * - tabWidth: The tab width.
 *
 * Returns:
 * - The next tab stop column, not checked for overflow.
 */
size_t cymbNextTab(size_t column, unsigned char tabWidth);

#endif
