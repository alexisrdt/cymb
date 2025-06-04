#include "cymb/arguments.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

CymbResult cymbParseArguments(const size_t argumentCount, const char* const* const arguments, CymbOptions* const options)
{
	CymbResult result = CYMB_SUCCESS;

	*options = (CymbOptions){
		.version = CYMB_C23,
		.tabWidth = 4
	};

	struct
	{
		bool version: 1;
		bool tabWidth: 1;
		bool debug: 1;
	} checks = {};

	for(size_t argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex)
	{
		if(strcmp(arguments[argumentIndex], "-o") == 0)
		{
			if(options->output)
			{
				fputs("Multiple outputs specified.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}

			++argumentIndex;
			if(argumentIndex == argumentCount)
			{
				fputs("Missing argument for -o.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}

			options->output = strdup(arguments[argumentIndex]);
			if(!options->output)
			{
				fputs("Failed to allocate memory.\n", stderr);
				result = CYMB_ERROR_OUT_OF_MEMORY;
				goto clear;
			}

			continue;
		}

		if(strncmp(arguments[argumentIndex], "-std=", 5) == 0)
		{
			if(checks.version)
			{
				fputs("Multiple versions specified.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}
			checks.version = true;

			const char* version = arguments[argumentIndex] + 5;
			if(version[0] != 'c' || version[1] == '\0' || version[2] == '\0' || version[3] != '\0')
			{
				fputs("Invalid version.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}

			++version;

			if(strcmp(version, "90") == 0)
			{
				options->version = CYMB_C90;
			}
			else if(strcmp(version, "99") == 0)
			{
				options->version = CYMB_C99;
			}
			else if(strcmp(version, "11") == 0)
			{
				options->version = CYMB_C11;
			}
			else if(strcmp(version, "17") == 0)
			{
				options->version = CYMB_C17;
			}
			else if(strcmp(version, "23") == 0)
			{
				options->version = CYMB_C23;
			}
			else
			{
				fputs("Invalid version.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}

			continue;
		}

		if(strncmp(arguments[argumentIndex], "-tab-width=", 11) == 0)
		{
			if(checks.tabWidth)
			{
				fputs("Multiple tab width specified.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}
			checks.tabWidth = true;

			const char* tabWidth = arguments[argumentIndex] + 11;
			options->tabWidth = 0;

			while(*tabWidth)
			{
				if(!isdigit((unsigned char)*tabWidth))
				{
					fputs("Invalid tab width.\n", stderr);
					result = CYMB_ERROR_INVALID_ARGUMENT;
					goto clear;
				}

				const unsigned char digit = *tabWidth - '0';
				if((UCHAR_MAX - digit) / 10 < options->tabWidth)
				{
					fputs("Invalid tab width.\n", stderr);
					result = CYMB_ERROR_INVALID_ARGUMENT;
					goto clear;
				}

				options->tabWidth *= 10;
				options->tabWidth += digit;

				++tabWidth;
			}

			if(tabWidth == 0)
			{
				fputs("Invalid tab width.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}

			continue;
		}

		if(strcmp(arguments[argumentIndex], "-g") == 0)
		{
			if(checks.debug)
			{
				fputs("Multiple debug specified.\n", stderr);
				result = CYMB_ERROR_INVALID_ARGUMENT;
				goto clear;
			}
			checks.debug = true;

			options->debug = true;

			continue;
		}

		if(arguments[argumentIndex][0] == '-')
		{
			fprintf(stderr, "Unknown option: %s\n", arguments[argumentIndex]);
			result = CYMB_ERROR_INVALID_ARGUMENT;
			goto clear;
		}

		if(options->input)
		{
			fputs("Multiple inputs specified.\n", stderr);
			result = CYMB_ERROR_INVALID_ARGUMENT;
			goto clear;
		}

		options->input = strdup(arguments[argumentIndex]);
		if(!options->input)
		{
			fputs("Failed to allocate memory.\n", stderr);
			result = CYMB_ERROR_OUT_OF_MEMORY;
			goto clear;
		}
	}

	if(!options->input)
	{
		fputs("No input specified.\n", stderr);
		result = CYMB_ERROR_INVALID_ARGUMENT;
		goto clear;
	}

	if(!options->output)
	{
		const char* const lastDot = strrchr(options->input, '.');
		const char* const lastSlash = strrchr(options->input, '/');
		const char* const lastBackslash = strrchr(options->input, '\\');

		const char* const lastSeparator = lastSlash > lastBackslash ? lastSlash : lastBackslash;

		const size_t outputLength = (lastDot > lastSeparator ? (size_t)(lastDot - options->input) : strlen(options->input)) + 2;
		options->output = malloc(outputLength + 1);
		if(!options->output)
		{
			fputs("Failed to allocate memory.\n", stderr);
			result = CYMB_ERROR_OUT_OF_MEMORY;
			goto clear;
		}

		strncpy(options->output, options->input, outputLength);
		options->output[outputLength - 2] = '.';
		options->output[outputLength - 1] = 's';
		options->output[outputLength] = '\0';
	}

	goto end;

	clear:
	free(options->input);
	free(options->output);

	end:
	return result;
}
