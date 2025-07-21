#include "cymb/cymb.h"

#include <stdio.h>
#include <stdlib.h>

/*
 * Read all the contents of a file in text mode.
 *
 * Parameters:
 * - path: Path of the file.
 * - string: A pointer to a string where to store the contents of the file.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_ERROR_OUT_OF_MEMORY if the file is too large.
 * - CYMB_ERROR_UNKNOWN otherwise.
 */
static CymbResult cymbReadFile(const char* const path, CymbString* const string)
{
	string->string = nullptr;
	string->length = 0;

	CymbResult result = CYMB_SUCCESS;

	// Open file.
	FILE* const file = fopen(path, "r");
	if(!file)
	{
		result = CYMB_ERROR_FILE_NOT_FOUND;
		goto error;
	}

	// Allocate initial buffer.
	size_t capacity = 1024;
	string->string = malloc(capacity);
	if(!string->string)
	{
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto error;
	}

	size_t toRead = capacity;
	while(true)
	{
		const size_t read = fread(string->string + string->length, sizeof(string->string[0]), toRead, file);
		string->length += read;

		if(read < toRead)
		{
			if(!feof(file))
			{
				result = CYMB_ERROR_UNKNOWN;
				goto error;
			}

			if(string->length >= cymbSizeMax)
			{
				result = CYMB_ERROR_OUT_OF_MEMORY;
				goto error;
			}

			char* const newString = realloc(string->string, string->length + 1);
			if(!newString)
			{
				result = CYMB_ERROR_OUT_OF_MEMORY;
				goto error;
			}
			string->string = newString;
			string->string[string->length] = '\0';

			result = fclose(file) == 0 ? CYMB_SUCCESS : CYMB_ERROR_UNKNOWN;
			goto end;
		}

		if(capacity >= cymbSizeMax)
		{
			result = CYMB_ERROR_OUT_OF_MEMORY;
			goto error;
		}

		const size_t newCapacity = capacity > cymbSizeMax / 2 ? cymbSizeMax : capacity * 2;
		toRead = newCapacity - capacity;
		capacity = newCapacity;

		char* const newString = realloc(string->string, capacity);
		if(!newString)
		{
			result = CYMB_ERROR_OUT_OF_MEMORY;
			goto error;
		}
		string->string = newString;
	}

	error:
	free(string->string);

	end:
	return result;
}

/*
 * Compile a source file.
 *
 * Parameters:
 * - options: A pointer to the options to use for compilation.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_FILE_NOT_FOUND if the file could not be opened.
 * - CYMB_ERROR_OUT_OF_MEMORY if the code is too large.
 * - CYMB_ERROR_UNKNOWN otherwise.
 */
static CymbResult cymbCompile(CymbDiagnosticList* const diagnostics)
{
	CymbResult result;

	// Get source code.
	CymbString source;
	result = cymbReadFile(diagnostics->file, &source);
	if(result != CYMB_SUCCESS)
	{
		switch(result)
		{
			case CYMB_ERROR_FILE_NOT_FOUND:
				fprintf(stderr, "Failed to open file \"%s\".\n", diagnostics->file);
				break;

			case CYMB_ERROR_OUT_OF_MEMORY:
				fputs("Out of memory.\n", stderr);
				break;

			default:
				fputs("Unknown error ocymbured.\n", stderr);
				break;
		}

		goto end;
	}

	CymbTokenList tokens;
	result = cymbLex(source.string, &tokens, diagnostics);
	if(result != CYMB_SUCCESS && result != CYMB_ERROR_INVALID_ARGUMENT)
	{
		goto clear;
	}

	CymbTree tree;
	const CymbResult parseResult = cymbParse(&(const CymbConstTokenList){
		.tokens = tokens.tokens,
		.count = tokens.count
	}, &tree, diagnostics);
	if(parseResult == CYMB_SUCCESS)
	{
		cymbFreeTree(&tree);
	}
	else
	{
		result = parseResult;
	}

	clear:
	cymbFreeTokenList(&tokens);

	cymbDiagnosticListPrint(&(const CymbConstDiagnosticList){
		.file = diagnostics->file,
		.tabWidth = diagnostics->tabWidth,
		.diagnostics = diagnostics->diagnostics,
		.count = diagnostics->count
	});

	free(source.string);

	end:
	return result;
}

CymbResult cymbMain(const CymbConstString* const arguments, const size_t argumentCount)
{
	CymbResult result = CYMB_SUCCESS;

	// Parse command line arguments.
	CymbDiagnosticList diagnostics;
	result = cymbDiagnosticListCreate(&diagnostics, nullptr, 4);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	CymbOptions options;
	result = cymbParseArguments(arguments, argumentCount, &options, &diagnostics);
	
	cymbDiagnosticListPrint(&(const CymbConstDiagnosticList){
		.diagnostics = diagnostics.diagnostics,
		.file = diagnostics.file,
		.tabWidth = diagnostics.tabWidth,
		.count = diagnostics.count
	});

	if(options.help)
	{
		cymbPrintHelp();
	}
	else if(options.version)
	{
		cymbPrintVersion();
	}
	
	if(result == CYMB_ERROR_OUT_OF_MEMORY)
	{
		goto end;
	}
	
	// Compile file.
	if(!options.help && !options.version)
	{
		for(size_t inputIndex = 0; inputIndex < options.inputCount; ++inputIndex)
		{
			diagnostics.count = 0;
			diagnostics.file = options.inputs[inputIndex];

			result = cymbCompile(&diagnostics);
			if(result == CYMB_ERROR_OUT_OF_MEMORY)
			{
				break;
			}
		}
	}

	CYMB_FREE(options.inputs);
	cymbDiagnosticListFree(&diagnostics);

	end:
	return result;
}
