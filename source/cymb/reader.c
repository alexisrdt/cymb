#include "cymb/reader.h"

#include <ctype.h>

/*
 * Get the next line bounds.
 *
 * Parameters:
 * - reader: A reader.
 * - string: Start of the line.
 */
static void cymbReaderLine(CymbReader* const reader, const char* string)
{
	reader->line.string = string;
	while(*string != '\0' && *string != '\n')
	{
		++string;
	}
	reader->line.length = string - reader->line.string;
}

void cymbReaderCreate(const char* const string, const unsigned char tabWidth, CymbReader* const reader)
{
	reader->string = string;

	reader->tabWidth = tabWidth;

	reader->position.line = 1;
	reader->position.column = 1;

	cymbReaderLine(reader, string);
}

void cymbReaderPop(CymbReader* const reader)
{
	switch(*reader->string)
	{
		case '\n':
			++reader->position.line;
			reader->position.column = 1;
			cymbReaderLine(reader, reader->string + 1);
			break;

		case '\t':
			reader->position.column = cymbNextTab(reader->position.column, reader->tabWidth);
			break;

		default:
			++reader->position.column;
			break;
	}

	++reader->string;
}

void cymbReaderSkip(CymbReader* const reader, size_t count)
{
	while(count--)
	{
		cymbReaderPop(reader);
	}
}

void cymbReaderSkipSpaces(CymbReader* const reader)
{
	while(isspace((unsigned char)*reader->string))
	{
		cymbReaderPop(reader);
	}
}
