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
static CymbResult cymbReadFile(const char* const path, CymbString* const string)
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
	size_t capacity = 1024;
	string->string = malloc(capacity);
	if(!string->string)
	{
		goto error;
	}

	size_t toRead = capacity;
	while(true)
	{
		const size_t read = fread(string->string + string->length, 1, toRead, file);
		string->length += read;

		if(read < toRead)
		{
			if(!feof(file))
			{
				goto error;
			}

			if(string->length >= cymbSizeMax)
			{
				goto error;
			}

			char* const newString = realloc(string->string, string->length + 1);
			if(!newString)
			{
				goto error;
			}
			string->string = newString;
			string->string[string->length] = '\0';

			if(fclose(file) != 0)
			{
				result = CYMB_OUT_OF_MEMORY;
				goto clear;
			}
			
			goto end;
		}

		if(capacity == cymbSizeMax)
		{
			goto error;
		}

		const size_t newCapacity = capacity > cymbSizeMax / 2 ? cymbSizeMax : capacity * 2;
		toRead = newCapacity - capacity;
		capacity = newCapacity;

		char* const newString = realloc(string->string, capacity);
		if(!newString)
		{
			goto error;
		}
		string->string = newString;
	}

	error:
	result = CYMB_OUT_OF_MEMORY;
	fclose(file);

	clear:
	free(string->string);
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
	result = cymbReadFile(diagnostics->file, &source);
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

	cymbFreeTree(&tree);
	cymbFreeTokenList(&tokens);

	clear:
	cymbDiagnosticListPrint(diagnostics);

	free(source.string);

	end:
	return result;
}

CymbResult cymbMain(const CymbConstString* const arguments, const size_t argumentCount)
{
	CymbArena arena;
	cymbArenaCreate(&arena);

	CymbDiagnosticList diagnostics;
	cymbDiagnosticListCreate(&diagnostics, &arena, nullptr, 8);

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
		goto clear;
	}
	
	// Compile file.
	for(size_t inputIndex = 0; inputIndex < options.inputCount; ++inputIndex)
	{
		cymbArenaClear(&arena);
		cymbDiagnosticListFree(&diagnostics);

		diagnostics.file = options.inputs[inputIndex];

		CymbResult fileResult = CYMB_SUCCESS;

		const size_t length = strlen(options.inputs[inputIndex]);
		if(length >= 2 && options.inputs[inputIndex][length - 2] == '.' && options.inputs[inputIndex][length - 1] == 's')
		{
			CymbString string;
			fileResult = cymbReadFile(options.inputs[inputIndex], &string);
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

			uint32_t* codes;
			size_t count;
			fileResult = cymbAssemble(string.string, &codes, &count, &diagnostics);

			cymbDiagnosticListPrint(&diagnostics);
			free(string.string);

			if(fileResult != CYMB_SUCCESS)
			{
				goto next;
			}

			if(length > cymbSizeMax - 3)
			{
				fileResult = CYMB_OUT_OF_MEMORY;

				free(codes);

				goto next;
			}

			const size_t outputLength = length + 2;
			char* const output = malloc(outputLength + 1);
			if(!output)
			{
				fileResult = CYMB_OUT_OF_MEMORY;

				free(codes);

				goto next;
			}

			strncpy(output, options.inputs[inputIndex], length - 1);
			output[length - 1] = 'b';
			output[length] = 'i';
			output[length + 1] = 'n';
			output[length + 2] = '\0';

			FILE* const file = fopen(output, "w");
			if(!file)
			{
				fileResult = CYMB_FILE_NOT_FOUND;

				fprintf(stderr, "Failed to open file \"%s\".\n", output);

				free(output);
				free(codes);

				goto next;
			}

			if(fwrite(codes, sizeof(codes[0]), count, file) != count)
			{
				fclose(file);
				free(output);
				free(codes);

				goto next;
			}

			if(fclose(file) != 0)
			{
				fileResult = CYMB_OUT_OF_MEMORY;
			}

			free(output);
			free(codes);

			goto next;
		}

		if(
			length >= 4 &&
			options.inputs[inputIndex][length - 4] == '.' &&
			options.inputs[inputIndex][length - 3] == 'b' &&
			options.inputs[inputIndex][length - 2] == 'i' &&
			options.inputs[inputIndex][length - 1] == 'n'
		)
		{
			FILE* const file = fopen(options.inputs[inputIndex], "rb");
			if(!file)
			{
				fileResult = CYMB_FILE_NOT_FOUND;
				fprintf(stderr, "Failed to open file \"%s\".\n", diagnostics.file);
				goto next;
			}

			if(fseek(file, 0, SEEK_END) != 0)
			{
				fclose(file);
				fileResult = CYMB_INVALID;
				goto next;
			}

			const long size = ftell(file);
			if(size <= 0 || size % 4 != 0)
			{
				fclose(file);
				fileResult = CYMB_INVALID;
				goto next;
			}
			if((unsigned long)size > cymbSizeMax)
			{
				fclose(file);
				fileResult = CYMB_OUT_OF_MEMORY;
				goto next;
			}

			rewind(file);

			const size_t count = size / 4;
			uint32_t* const codes = malloc(size);
			if(!codes)
			{
				fclose(file);
				fileResult = CYMB_INVALID;
				goto next;
			}

			if(fread(codes, 4, count, file) != count)
			{
				fclose(file);
				free(codes);
				fileResult = CYMB_INVALID;
				goto next;
			}

			if(fclose(file) != 0)
			{
				free(codes);
				fileResult = CYMB_INVALID;
				goto next;
			}

			CymbString assembly;
			fileResult = cymbDisassemble(codes, count, &assembly, &diagnostics);

			free(codes);

			cymbDiagnosticListPrint(&diagnostics);

			if(fileResult != CYMB_SUCCESS)
			{
				free(assembly.string);
				goto next;
			}

			fputs(assembly.string, stdout);
			free(assembly.string);

			goto next;
		}

		fileResult = cymbCompile(&arena, &diagnostics);

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

	clear:
	free(options.inputs);
	cymbArenaFree(&arena);

	return result;
}
