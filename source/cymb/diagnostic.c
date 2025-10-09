#include "cymb/diagnostic.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	if(diagnostic->info.line.string)
	{
		fprintf(stderr, "In file %s, line %zu, column %zu:\n", file, diagnostic->info.position.line, diagnostic->info.position.column);
	}

	switch(diagnostic->type)
	{
		case CYMB_TOO_MANY_INPUTS:
			fputs("Too many inputs.\n", stderr);
			break;

		case CYMB_UNKNOWN_OPTION:
			fputs("Unknown option.\n", stderr);
			break;

		case CYMB_MISSING_ARGUMENT:
			fputs("Missing argument.\n", stderr);
			break;

		case CYMB_UNEXPECTED_ARGUMENT:
			fputs("Unexpected argument.\n", stderr);
			break;

		case CYMB_INVALID_ARGUMENT:
			fputs("Invalid argument.\n", stderr);
			break;

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

		case CYMB_SEPARATOR_AFTER_BASE:
			fputs("Digit separator after base prefix.\n", stderr);
			break;

		case CYMB_DUPLICATE_SEPARATORS:
			fputs("Duplicate digit separators.\n", stderr);
			break;

		case CYMB_TRAILING_SEPARATOR:
			fputs("Trailing digit separator.\n", stderr);
			break;

		case CYMB_UNEXPECTED_TOKEN:
			fputs("Unexpected token.\n", stderr);
			break;

		case CYMB_UNMATCHED_PARENTHESIS:
			fputs("Unmatched parenthesis.\n", stderr);
			break;

		case CYMB_UNMATCHED_BRACE:
			fputs("Unmatched brace.\n", stderr);
			break;

		case CYMB_MULTIPLE_CONST:
			fputs("Multiple const specification.\n", stderr);
			break;

		case CYMB_MULTIPLE_RESTRICT:
			fputs("Multiple restrict specification.\n", stderr);
			break;

		case CYMB_MULTIPLE_STATIC:
			fputs("Multiple static specification.\n", stderr);
			break;

		case CYMB_MISSING_TYPE:
			fputs("Missing type.\n", stderr);
			break;

		case CYMB_EXPECTED_EXPRESSION:
			fputs("Expected expression.\n", stderr);
			break;

		case CYMB_INVALID_DECLARATION:
			fputs("Invalid declaration.\n", stderr);
			break;

		case CYMB_EXPECTED_PARENTHESIS:
			fputs("Expected parenthesis.\n", stderr);
			break;

		case CYMB_EXPECTED_SEMICOLON:
			fputs("Expected semicolon.\n", stderr);
			break;

		case CYMB_EXPECTED_FUNCTION:
			fputs("Expected function.\n", stderr);
			break;

		case CYMB_EXPECTED_PARAMETER:
			fputs("Expected parameter.\n", stderr);
			break;

		case CYMB_INVALID_TYPE:
			fputs("Invalid type.\n", stderr);
			break;

		case CYMB_UNKNOWN_INSTRUCTION:
			fputs("Unknown instruction.\n", stderr);
			break;

		case CYMB_UNEXPECTED_CHARACTERS_AFTER_INSTRUCTION:
			fputs("Unexpected characters after instruction.\n", stderr);
			break;

		case CYMB_MISSING_SPACE:
			fputs("Missing space.\n", stderr);
			break;

		case CYMB_MISSING_COMMA:
			fputs("Missing comma.\n", stderr);
			break;

		case CYMB_EXPECTED_REGISTER:
			fputs("Expected a register.\n", stderr);
			break;

		case CYMB_EXPECTED_IMMEDIATE:
			fputs("Expected an immediate.\n", stderr);
			break;

		case CYMB_INVALID_REGISTER:
			fputs("Invalid register.\n", stderr);
			break;

		case CYMB_EXPECTED_SP:
			fputs("Expected an SP register.\n", stderr);
			break;

		case CYMB_INVALID_SP:
			fputs("Invalid SP register.\n", stderr);
			break;

		case CYMB_INVALID_ZR:
			fputs("Invalid ZR register.\n", stderr);
			break;

		case CYMB_INVALID_REGISTER_WIDTH:
			fputs("Invalid register width.\n", stderr);
			break;

		case CYMB_INVALID_IMMEDIATE:
			fputs("Invalid immediate.\n", stderr);
			break;

		case CYMB_INVALID_EXTENSION:
			fputs("Invalid extension.\n", stderr);
			break;

		case CYMB_DUPLICATE_LABEL:
			fputs("Duplicate label.\n", stderr);
			break;

		case CYMB_INVALID_LABEL:
			fputs("Invalid label.\n", stderr);
			break;

		default:
			unreachable();
	}

	if(diagnostic->info.line.string)
	{
		int written = fprintf(stderr, "%zu | ", diagnostic->info.position.line);

		size_t column = 1;
		for(size_t characterIndex = 0; characterIndex < diagnostic->info.line.length; ++characterIndex)
		{
			if(diagnostic->info.line.string[characterIndex] == '\t')
			{
				const size_t nextTab = cymbNextTab(column, tabWidth);
				size_t offset = nextTab - column;
				while(offset--)
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

		while(written--)
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
				size_t offset = nextTab - column;
				while(offset--)
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
	else if(diagnostic->info.hint.string)
	{
		fputs(diagnostic->info.hint.string, stderr);
		fputc('\n', stderr);
	}
}

void cymbDiagnosticListPrint(const CymbDiagnosticList* const diagnostics)
{
	const CymbDiagnostic* diagnostic = diagnostics->start;
	while(diagnostic)
	{
		cymbDiagnosticPrint(diagnostic, diagnostics->file, diagnostics->tabWidth);
		diagnostic = diagnostic->next;
	}
}

void cymbDiagnosticListCreate(CymbDiagnosticList* const diagnostics, CymbArena* const arena, const char* const file, const unsigned char tabWidth)
{
	*diagnostics = (CymbDiagnosticList){
		.file = file,
		.tabWidth = tabWidth,
		.arena = arena
	};
}

void cymbDiagnosticListFree(CymbDiagnosticList* const diagnostics)
{
	diagnostics->start = nullptr;
	diagnostics->end = nullptr;
}

CymbResult cymbDiagnosticAdd(CymbDiagnosticList* const diagnostics, const CymbDiagnostic* const diagnostic)
{
	CymbDiagnostic* const result = cymbArenaGet(diagnostics->arena, sizeof(*diagnostic), alignof(typeof(*diagnostic)));
	if(!result)
	{
		return CYMB_OUT_OF_MEMORY;
	}

	*result = *diagnostic;
	result->next = nullptr;

	if(!diagnostics->start)
	{
		diagnostics->start = result;
		diagnostics->end = result;
	}
	else
	{
		diagnostics->end->next = result;
		diagnostics->end = result;
	}

	return CYMB_SUCCESS;
}

size_t cymbNextTab(const size_t column, const unsigned char tabWidth)
{
	return column + tabWidth - (column - 1) % tabWidth;
}
