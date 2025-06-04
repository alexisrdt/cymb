#include "cymb/cymb.h"

#include <stdlib.h>

// Initial buffer size for file reading.
static constexpr size_t cymbInitialSize = 1024;

CymbResult cymbReadFile(const char* const path, CymbString* const string)
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
	size_t capacity = cymbInitialSize;
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

CymbResult cymbCompile(const CymbOptions* const options)
{
	CymbResult result;

	// Get source code.
	CymbString source;
	result = cymbReadFile(options->input, &source);
	if(result != CYMB_SUCCESS)
	{
		switch(result)
		{
			case CYMB_ERROR_FILE_NOT_FOUND:
				fprintf(stderr, "Failed to open file \"%s\".\n", options->input);
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

	CymbDiagnosticList diagnostics;
	result = cymbDiagnosticListCreate(&diagnostics, options->input, options->tabWidth);
	if(result != CYMB_SUCCESS)
	{
		goto clear;
	}

	CymbTokenList tokens;
	result = cymbLex(source.string, &tokens, &diagnostics);
	if(result != CYMB_SUCCESS && result != CYMB_ERROR_INVALID_ARGUMENT)
	{
		goto print_clear;
	}

	CymbTree tree;
	const CymbResult parseResult = cymbParse(&(const CymbConstTokenList){
		.tokens = tokens.tokens,
		.count = tokens.count
	}, &tree, &diagnostics);
	if(parseResult == CYMB_SUCCESS)
	{
		cymbFreeTree(&tree);
	}
	else
	{
		result = parseResult;
	}

	print_clear:
	cymbFreeTokenList(&tokens);

	cymbDiagnosticListPrint(&(const CymbConstDiagnosticList){
		.file = diagnostics.file,
		.tabWidth = diagnostics.tabWidth,
		.diagnostics = diagnostics.diagnostics,
		.count = diagnostics.count
	});
	cymbDiagnosticListFree(&diagnostics);

	clear:
	free(source.string);

	end:
	return result;
}
