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
		cymbPrintHelp();
		result = CYMB_ERROR_INVALID_ARGUMENT;
		goto end;
	}

	--argumentCount;
	++arguments;

	// Transform arguments.
	const size_t stringArgumentCount = argumentCount;

	CymbConstString* stringArguments;
	constexpr size_t maxStringArgumentCount = cymbSizeMax / sizeof(stringArguments[0]);

	if(stringArgumentCount > maxStringArgumentCount)
	{
		fputs("Too many arguments.\n", stderr);
		result = CYMB_ERROR_INVALID_ARGUMENT;
		goto end;
	}

	stringArguments = malloc(stringArgumentCount * sizeof(stringArguments[0]));
	if(!stringArguments)
	{
		fputs("Out of memory.\n", stderr);
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto end;
	}

	for(size_t argumentIndex = 0; argumentIndex < stringArgumentCount; ++argumentIndex)
	{
		stringArguments[argumentIndex].string = arguments[argumentIndex];
		stringArguments[argumentIndex].length = strlen(stringArguments[argumentIndex].string);

		if(stringArguments[argumentIndex].length >= cymbSizeMax - 1)
		{
			fputs("Argument too long.\n", stderr);
			result = CYMB_ERROR_INVALID_ARGUMENT;
			goto clear;
		}
	}

	// Run Cymb.
	result = cymbMain(stringArguments, stringArgumentCount);
	if(result == CYMB_ERROR_OUT_OF_MEMORY)
	{
		fputs("Out of memory.\n", stderr);
	}

	clear:
	free(stringArguments);

	end:
	return result == CYMB_SUCCESS ? EXIT_SUCCESS : EXIT_FAILURE;
}
