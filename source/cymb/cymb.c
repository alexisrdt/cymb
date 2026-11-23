#include "cymb/cymb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Read all the contents of a file in text mode.
 *
 * Parameters:
 * - path: Path of the file.
 * - string: A string where to store the contents of the file.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_OUT_OF_MEMORY otherwise.
 */
static CymbResult cymbReadFile(CymbArena* const arena, const char* const path, CymbString* const string)
{
	CymbResult result = CYMB_SUCCESS;

	*string = (CymbString){};

	// Open file.
	FILE* const file = fopen(path, "r");
	if(!file)
	{
		result = CYMB_FILE_NOT_FOUND;
		goto end;
	}

	// Allocate initial buffer.
	CymbSegmentedArray stringArray;
	cymbSegmentedArrayCreate(&stringArray, arena, sizeof(string->string[0]), alignof(typeof(string->string[0])));

	while(true)
	{
		if(!cymbSegmentArrayNext(&stringArray))
		{
			goto error;
		}

		stringArray.end->size = fread(stringArray.end->segment, stringArray.elementSize, stringArray.end->capacity, file);
		string->length += stringArray.end->size;

		if(stringArray.end->size < stringArray.end->capacity)
		{
			if(!feof(file))
			{
				goto error;
			}

			if(string->length >= cymbArenaSizeMax / sizeof(string->string[0]))
			{
				goto error;
			}

			string->string = cymbArenaAllocate(arena, (string->length + 1) * sizeof(string->string[0]), alignof(typeof(string->string[0])));
			if(!string->string)
			{
				goto error;
			}
			cymbSegmentedArrayCopy(&stringArray, string->string);
			string->string[string->length] = '\0';

			if(fclose(file) != 0)
			{
				result = CYMB_OUT_OF_MEMORY;
				goto clear;
			}
			
			goto end;
		}
	}

	error:
	result = CYMB_OUT_OF_MEMORY;
	fclose(file);

	clear:
	*string = (CymbString){};

	end:
	return result;
}

/*
 * Compile a source file.
 *
 * Parameters:
 * - arena: An arena to use for allocations.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_OUT_OF_MEMORY if the code is too large.
 */
static CymbResult cymbCompile(CymbArena* const arena, CymbDiagnosticList* const diagnostics)
{
	CymbResult result;

	// Get source code.
	CymbString source;
	result = cymbReadFile(arena, diagnostics->file, &source);
	if(result != CYMB_SUCCESS)
	{
		switch(result)
		{
			case CYMB_FILE_NOT_FOUND:
				fprintf(stderr, "Failed to open file \"%s\".\n", diagnostics->file);
				break;

			case CYMB_OUT_OF_MEMORY:
				fputs("Out of memory.\n", stderr);
				break;

			default:
				unreachable();
		}

		goto end;
	}

	CymbTokenList tokens;
	result = cymbLex(source.string, &tokens, diagnostics);
	if(result != CYMB_SUCCESS && result != CYMB_INVALID)
	{
		goto clear;
	}

	CymbTree tree;
	const CymbResult parseResult = cymbParse(&tokens, arena, &tree, diagnostics);
	if(parseResult != CYMB_SUCCESS)
	{
		result = parseResult;
	}

	clear:
	cymbDiagnosticListPrint(diagnostics);

	end:
	return result;
}

CymbResult cymbMain(CymbArena* const arena, const CymbConstString* const arguments, const size_t argumentCount)
{
	CymbDiagnosticList diagnostics;
	cymbDiagnosticListCreate(&diagnostics, arena, nullptr, 8);

	// Parse arguments.
	CymbOptions options;
	CymbResult result = cymbParseArguments(arguments, argumentCount, &options, &diagnostics);
	diagnostics.tabWidth = options.tabWidth;

	cymbDiagnosticListPrint(&diagnostics);

	if(options.help || (options.inputCount == 0 && !options.version))
	{
		cymbPrintHelp();
	}
	else if(options.version)
	{
		cymbPrintVersion();
	}
	
	if(result != CYMB_SUCCESS || options.help || options.version)
	{
		goto end;
	}
	
	// Compile file.
	for(size_t inputIndex = 0; inputIndex < options.inputCount; ++inputIndex)
	{
		diagnostics.file = options.inputs[inputIndex];

		CymbResult fileResult = CYMB_SUCCESS;

		const size_t length = strlen(options.inputs[inputIndex]);
		if(length >= 2 && options.inputs[inputIndex][length - 2] == '.' && options.inputs[inputIndex][length - 1] == 's')
		{
			CymbString string;
			fileResult = cymbReadFile(arena, options.inputs[inputIndex], &string);
			if(fileResult != CYMB_SUCCESS)
			{
				switch(result)
				{
					case CYMB_FILE_NOT_FOUND:
						fprintf(stderr, "Failed to open file \"%s\".\n", diagnostics.file);
						break;

					case CYMB_OUT_OF_MEMORY:
						fputs("Out of memory.\n", stderr);
						break;

					default:
						unreachable();
				}

				goto next;
			}

			char* codes;
			size_t size;
			fileResult = cymbAssemble(string.string, &codes, &size, options.machine, &diagnostics);

			cymbDiagnosticListPrint(&diagnostics);

			goto next;
		}

		fileResult = cymbCompile(arena, &diagnostics);

		next:
		if(
			(fileResult == CYMB_OUT_OF_MEMORY) ||
			((result == CYMB_SUCCESS || result == CYMB_FILE_NOT_FOUND) && fileResult == CYMB_INVALID) ||
			(result == CYMB_SUCCESS && fileResult == CYMB_FILE_NOT_FOUND)
		)
		{
			result = fileResult;
		}

		if(result == CYMB_OUT_OF_MEMORY)
		{
			break;
		}
	}

	end:
	return result;
}
