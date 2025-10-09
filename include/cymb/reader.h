#ifndef CYMB_READER_H
#define CYMB_READER_H

#include "cymb/diagnostic.h"

/*
 * A string reader.
 *
 * Fields:
 * - string: A string to read.
 * - tabWidth: The tab width to use when encoutering tab characters.
 * - position: The position of the current character.
 * - line: The current line.
 */
typedef struct CymbReader
{
	const char* string;

	unsigned char tabWidth;

	CymbPosition position;

	CymbStringView line;
} CymbReader;

/*
 * Create a reader.
 *
 * Parameters:
 * - string: A string to read.
 * - tabWidth: The tab width to use.
 * - reader: The reader.
 */
void cymbReaderCreate(const char* string, unsigned char tabWidth, CymbReader* reader);

/*
 * Move a reader one character forward.
 *
 * Parameters:
 * - reader: A reader.
 */
void cymbReaderPop(CymbReader* reader);

/*
 * Skip characters in a reader.
 *
 * Parameters:
 * - reader: A reader.
 * - count: The number of characters to skip.
 */
void cymbReaderSkip(CymbReader* reader, size_t count);

/*
 * Skip line in a reader.
 *
 * Parameters:
 * - reader: A reader.
 */
void cymbReaderSkipLine(CymbReader* reader);

/*
 * Skip spaces in a reader.
 *
 * Parameters:
 * - reader: A reader.
 */
void cymbReaderSkipSpaces(CymbReader* reader);

/*
 * Skip spaces in a line in a reader.
 *
 * Parameters:
 * - reader: A reader.
 */
void cymbReaderSkipSpacesInLine(CymbReader* reader);

#endif
