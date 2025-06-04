#include "cymb/diagnostic.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Print a diagnostic.
 *
 * Parameters:
 * - diagnostic: A diagnostic.
 * - file: The file for which the diagnostic was emitted.
 * - tabWidth: The tab width to use for printing.
 */
static void cymbDiagnosticPrint(const CymbDiagnostic* const diagnostic, const char* const file, const unsigned char tabWidth)
{
	fprintf(stderr, "In file %s, line %zu, column %zu:\n", file, diagnostic->info.position.line, diagnostic->info.position.column);

	switch(diagnostic->type)
	{
		case CYMB_UNKNOWN_TOKEN:
			fputs("Unknown token.\n", stderr);
			break;

		case CYMB_INVALID_CONSTANT_SUFFIX:
			fputs("Invalid constant suffix.\n", stderr);
			break;

		case CYMB_INVALID_CHARACTER_CONSTANT:
			fputs("Invalid character constant.\n", stderr);
			break;

		case CYMB_INVALID_STRING_CHARACTER:
			fputs("Invalid character in string.\n", stderr);
			break;

		case CYMB_UNFINISHED_STRING:
			fputs("Unfinished string.\n", stderr);
			break;

		case CYMB_CONSTANT_TOO_LARGE:
			fputs("Integer constant too large.\n", stderr);
			break;

		case CYMB_UNEXPECTED_TOKEN:
			fputs("Unexpected token.\n", stderr);
			break;

		case CYMB_UNMATCHED_PARENTHESIS:
			fputs("Unmatched parenthesis.\n", stderr);
			break;

		case CYMB_DOUBLE_CONST:
			fputs("Double const specification.\n", stderr);
			break;

		case CYMB_DOUBLE_RESTRICT:
			fputs("Double restrict specification.\n", stderr);
			break;

		default:
			unreachable();
	}

	const int written = fprintf(stderr, "%zu | ", diagnostic->info.position.line);

	size_t column = 1;
	for(size_t characterIndex = 0; characterIndex < diagnostic->info.line.length; ++characterIndex)
	{
		if(diagnostic->info.line.string[characterIndex] == '\t')
		{
			const size_t nextTab = cymbNextTab(column, tabWidth);

			for(size_t space = column; space < nextTab; ++space)
			{
				fputc(' ', stderr);
			}

			column = nextTab;
		}
		else
		{
			fputc(diagnostic->info.line.string[characterIndex], stderr);
			++column;
		}
	}
	fputc('\n', stderr);

	for(int prefixIndex = 0; prefixIndex < written; ++prefixIndex)
	{
		fputc(' ', stderr);
	}

	column = 1;

	const ptrdiff_t offset = diagnostic->info.hint.string - diagnostic->info.line.string;
	for(ptrdiff_t offsetIndex = 0; offsetIndex < offset; ++offsetIndex)
	{
		if(diagnostic->info.line.string[offsetIndex] == '\t')
		{
			const size_t nextTab = cymbNextTab(column, tabWidth);

			for(size_t space = column; space < nextTab; ++space)
			{
				fputc(' ', stderr);
			}

			column = nextTab;
		}
		else
		{
			fputc(' ', stderr);
			++column;
		}
	}

	for(size_t hintIndex = 0; hintIndex < diagnostic->info.hint.length; ++hintIndex)
	{
		fputc('~', stderr);
	}
	fputc('\n', stderr);
}

void cymbDiagnosticListPrint(const CymbConstDiagnosticList* const diagnostics)
{
	for(size_t diagnosticIndex = 0; diagnosticIndex < diagnostics->count; ++diagnosticIndex)
	{
		cymbDiagnosticPrint(&diagnostics->diagnostics[diagnosticIndex], diagnostics->file, diagnostics->tabWidth);
	}
}

CymbResult cymbDiagnosticListCreate(CymbDiagnosticList* const diagnostics, const char* const file, const unsigned char tabWidth)
{
	diagnostics->capacity = 8;
	diagnostics->count = 0;

	diagnostics->diagnostics = malloc(diagnostics->capacity * sizeof(diagnostics->diagnostics[0]));
	if(!diagnostics->diagnostics)
	{
		diagnostics->capacity = 0;
		return CYMB_ERROR_OUT_OF_MEMORY;
	}

	diagnostics->file = file;
	diagnostics->tabWidth = tabWidth;

	return CYMB_SUCCESS;
}

void cymbDiagnosticListFree(CymbDiagnosticList* const diagnostics)
{
	free(diagnostics->diagnostics);
}

CymbResult cymbDiagnosticAdd(CymbDiagnosticList* const diagnostics, const CymbDiagnostic* const diagnostic)
{
	if(diagnostics->count == diagnostics->capacity)
	{
		if(diagnostics->capacity == cymbSizeMax)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}

		const size_t newCapacity = diagnostics->capacity > cymbSizeMax / 2 ? cymbSizeMax : diagnostics->capacity * 2;

		CymbDiagnostic* const newDiagnostics = realloc(diagnostics->diagnostics, newCapacity);
		if(!newDiagnostics)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}

		diagnostics->diagnostics = newDiagnostics;
		diagnostics->capacity = newCapacity;
	}

	diagnostics->diagnostics[diagnostics->count] = *diagnostic;
	++diagnostics->count;

	return CYMB_SUCCESS;
}

size_t cymbNextTab(size_t column, unsigned char tabWidth)
{
	return column + tabWidth - (column - 1) % tabWidth;
}
