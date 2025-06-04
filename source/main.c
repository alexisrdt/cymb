#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/cymb.h"

int main(int argumentCount, char** arguments)
{
	CymbResult result = CYMB_SUCCESS;

	// Check that there are command line arguments.
	if(argumentCount <= 1)
	{
		result = CYMB_ERROR_INVALID_ARGUMENT;
		goto end;
	}

	constexpr size_t maxArgumentCount = cymbSizeMax / sizeof(arguments[0]);

#if INT_MAX <= SIZE_MAX
	if(maxArgumentCount < (size_t)argumentCount)
#else
	if((unsigned int)maxArgumentCount < (unsigned int)argumentCount)
#endif
	{
		fputs("Too many arguments.\n", stderr);
		result = CYMB_ERROR_INVALID_ARGUMENT;
		goto end;
	}

	--argumentCount;
	++arguments;

	const size_t argumentsSize = argumentCount * sizeof(arguments[0]);
	const char** const argumentsConst = malloc(argumentsSize);
	if(!argumentsConst)
	{
		fputs("Out of memory.\n", stderr);
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto end;
	}
	for(size_t argumentIndex = 0; argumentIndex < (size_t)argumentCount; ++argumentIndex)
	{
		const size_t length = strlen(arguments[argumentIndex]);
		if(length >= cymbSizeMax - 1)
		{
			fputs("Argument too long.\n", stderr);
			free(argumentsConst);
			goto end;
		}
		argumentsConst[argumentIndex] = arguments[argumentIndex];
	}

	// Parse command line arguments.
	CymbOptions options;
	result = cymbParseArguments(argumentCount, argumentsConst, &options);
	free(argumentsConst);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	// Compile file.
	result = cymbCompile(&options);

	free(options.input);
	free(options.output);

	end:
	return result == CYMB_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
