#ifndef CYMB_DIAGNOSTIC_H
#define CYMB_DIAGNOSTIC_H

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
	CYMB_DUPLICATE_SEPARATORS,
	CYMB_TRAILING_SEPARATOR,
	// Nodes.
	CYMB_UNEXPECTED_TOKEN,
	CYMB_UNMATCHED_PARENTHESIS,
	CYMB_UNMATCHED_BRACE,
	CYMB_MULTIPLE_CONST,
	CYMB_MULTIPLE_RESTRICT,
	CYMB_MULTIPLE_STATIC,
	CYMB_MISSING_TYPE,
	CYMB_INVALID_TYPE,
	CYMB_EXPECTED_EXPRESSION,
	CYMB_INVALID_DECLARATION,
	CYMB_EXPECTED_PARENTHESIS,
	CYMB_EXPECTED_SEMICOLON,
	CYMB_EXPECTED_FUNCTION,
	CYMB_EXPECTED_PARAMETER,
	// Assembly.
	CYMB_UNKNOWN_INSTRUCTION,
	CYMB_UNEXPECTED_CHARACTERS_AFTER_INSTRUCTION,
	CYMB_MISSING_SPACE,
	CYMB_MISSING_COMMA,
	CYMB_EXPECTED_REGISTER,
	CYMB_EXPECTED_IMMEDIATE,
	CYMB_INVALID_REGISTER,
	CYMB_EXPECTED_SP,
	CYMB_INVALID_SP,
	CYMB_INVALID_ZR,
	CYMB_INVALID_REGISTER_WIDTH,
	CYMB_INVALID_IMMEDIATE,
	CYMB_INVALID_EXTENSION,
	CYMB_DUPLICATE_LABEL,
	CYMB_INVALID_LABEL
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
 * - next: The next diagnostic.
 */
typedef struct CymbDiagnostic
{
	CymbDiagnosticType type;
	CymbDiagnosticInfo info;

	struct CymbDiagnostic* next;
} CymbDiagnostic;

/*
 * A list of diagnostics.
 *
 * Fields:
 * - file: The file for which diagnostics are emitted.
 * - tabWidth: The tab width used for diagnostics.
 * - arena: The arena used for allocations.
 * - start: The first diagnostic.
 * - end: The last diagnostic.
 */
typedef struct CymbDiagnosticList
{
	const char* file;
	unsigned char tabWidth;

	CymbArena* arena;
	CymbDiagnostic* start;
	CymbDiagnostic* end;
} CymbDiagnosticList;

/*
 * Print diagnostics.
 *
 * Parameters:
 * - diagnostics: The diagnostics to print.
 */
void cymbDiagnosticListPrint(const CymbDiagnosticList* diagnostics);

/*
 * Create a diagnostic list.
 *
 * Parameters:
 * - diagnostics: A pointer to the list to create.
 * - arena: The arena to use for allocations.
 * - file: The file for which diagnostics are emitted.
 * - tabWidth: The tab width used for diagnostics.
 */
void cymbDiagnosticListCreate(CymbDiagnosticList* diagnostics, CymbArena* arena, const char* file, unsigned char tabWidth);

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
 * - CYMB_OUT_OF_MEMORY if the diagnostic could not be added.
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
