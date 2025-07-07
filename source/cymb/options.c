#include "cymb/options.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"
#include "cymb/version.h"

typedef enum CymbOption
{
	CYMB_OPTION_DEBUG,
	CYMB_OPTION_HELP,
	CYMB_OPTION_OUTPUT,
	CYMB_OPTION_STANDARD,
	CYMB_OPTION_TAB_WIDTH,
	CYMB_OPTION_VERSION
} CymbOption;

typedef struct CymbLongOption
{
	CymbConstString name;
	bool argument;
} CymbLongOption;

const CymbLongOption longOptions[] = {
	{CYMB_STRING("debug"), false},
	{CYMB_STRING("help"), false},
	{CYMB_STRING("output"), true},
	{CYMB_STRING("standard"), true},
	{CYMB_STRING("tab-width"), true},
	{CYMB_STRING("version"), false}
};
constexpr size_t longOptionCount = CYMB_LENGTH(longOptions);
constexpr size_t longOptionSize = sizeof(longOptions[0]);

typedef struct CymbShortOption
{
	char name;
	size_t longOptionIndex;
} CymbShortOption;

const CymbShortOption shortOptions[] = {
	{'g', CYMB_OPTION_DEBUG},
	{'h', CYMB_OPTION_HELP},
	{'o', CYMB_OPTION_OUTPUT},
	{'v', CYMB_OPTION_VERSION}
};
constexpr size_t shortOptionCount = CYMB_LENGTH(shortOptions);
constexpr size_t shortOptionSize = sizeof(shortOptions[0]);

static int cymbCompareShortOptions(const void* const characterVoid, const void* const optionVoid)
{
	const char character = *(const char*)characterVoid;
	const CymbShortOption* const option = optionVoid;

	return (character > option->name) - (character < option->name);
}

static int cymbCompareLongOptions(const void* const stringVoid, const void* const optionVoid)
{
	const CymbConstString* const string = stringVoid;
	const CymbLongOption* const option = optionVoid;

	const int result = strncmp(string->string, option->name.string, string->length);
	return result == 0 ? (string->length > option->name.length) - (string->length < option->name.length) : result;
}

static CymbResult cymbApplyOption(CymbOptions* const options, const CymbOption option, const CymbConstString* const argument, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	switch(option)
	{
		case CYMB_OPTION_HELP:
			options->help = true;
			break;

		case CYMB_OPTION_VERSION:
			options->version = true;
			break;

		case CYMB_OPTION_DEBUG:
			options->debug = true;
			break;

		case CYMB_OPTION_OUTPUT:
			options->output = argument->string;
			break;

		case CYMB_OPTION_STANDARD:
			if(*argument->string != 'c' || argument->length != 3)
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				result = cymbDiagnosticAdd(diagnostics, &diagnostic);

				result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;

				break;
			}

			if(argument->string[1] == '9' && argument->string[2] == '0')
			{
				options->standard = CYMB_C90;
			}
			else if(argument->string[1] == '9' && argument->string[2] == '5')
			{
				options->standard = CYMB_C95;
			}
			else if(argument->string[1] == '9' && argument->string[2] == '9')
			{
				options->standard = CYMB_C99;
			}
			else if(argument->string[1] == '1' && argument->string[2] == '1')
			{
				options->standard = CYMB_C11;
			}
			else if(argument->string[1] == '1' && argument->string[2] == '7')
			{
				options->standard = CYMB_C17;
			}
			else if(argument->string[1] == '2' && argument->string[2] == '3')
			{
				options->standard = CYMB_C23;
			}
			else
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				result = cymbDiagnosticAdd(diagnostics, &diagnostic);

				result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;

				break;
			}

			break;

		case CYMB_OPTION_TAB_WIDTH:
			if(!isdigit((unsigned char)*argument->string))
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				result = cymbDiagnosticAdd(diagnostics, &diagnostic);

				result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;

				break;
			}

			char* digitsEnd;
			const unsigned long tabWidth = strtoul(argument->string, &digitsEnd, 10);

			if(tabWidth == 0 || tabWidth > 100 || digitsEnd == argument->string || *digitsEnd != '\0')
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				result = cymbDiagnosticAdd(diagnostics, &diagnostic);

				result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;

				break;
			}

			options->tabWidth = tabWidth;

			break;

		default:
			unreachable();
	}

	return result;
}

typedef struct CymbArgumentsParser
{
	const CymbConstString* arguments;
	size_t argumentCount;

	CymbOptions* options;
	CymbDiagnosticList* diagnostics;
} CymbArgumentsParser;

static CymbResult cymbParseShortOptions(CymbArgumentsParser* const parser)
{
	CymbResult result = CYMB_SUCCESS;

	const char* argument = parser->arguments->string + 1;

	while(*argument != '\0')
	{
		const CymbShortOption* const shortOption = bsearch(argument, shortOptions, shortOptionCount, shortOptionSize, cymbCompareShortOptions);
		if(!shortOption)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNKNOWN_OPTION,
				.info = {
					.hint = {argument, 1}
				}
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
				goto end;
			}

			result = CYMB_ERROR_INVALID_ARGUMENT;

			goto next_argument;
		}

		const CymbLongOption* const option = &longOptions[shortOption->longOptionIndex];

		CymbConstString optionArgument = {};
		if(option->argument)
		{
			++argument;
			if(*argument != '\0')
			{
				optionArgument = (CymbConstString){argument, parser->arguments->length - (argument - parser->arguments->string)};

				goto apply;
			}

			++parser->arguments;
			--parser->argumentCount;
			if(parser->argumentCount == 0)
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_MISSING_ARGUMENT,
					.info = {
						.hint = *(parser->arguments - 1)
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
					goto end;
				}

				result = CYMB_ERROR_INVALID_ARGUMENT;

				continue;
			}

			optionArgument = *parser->arguments;
		}

		apply:
		const CymbResult applyResult = cymbApplyOption(parser->options, option - longOptions, &optionArgument, parser->diagnostics);
		if(applyResult == CYMB_ERROR_OUT_OF_MEMORY)
		{
			goto end;
		}
		else if(applyResult == CYMB_ERROR_INVALID_ARGUMENT)
		{
			result = applyResult;
		}

		if(optionArgument.string)
		{
			continue;
		}

		next_argument:
		++argument;
	}

	end:
	return result;
}

static CymbResult cymbParseLongOption(CymbArgumentsParser* const parser)
{
	CymbResult result = CYMB_SUCCESS;

	const CymbConstString argument = {
		.string = parser->arguments->string + 2,
		.length = parser->arguments->length - 2
	};

	const char* const equal = strchr(argument.string, '=');

	const CymbConstString optionString = {
		.string = argument.string,
		.length = equal ? (size_t)(equal - argument.string) : argument.length
	};

	const CymbLongOption* const option = bsearch(&optionString, longOptions, longOptionCount, longOptionSize, cymbCompareLongOptions);
	if(!option)
	{
		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNKNOWN_OPTION,
			.info = {
				.hint = argument
			}
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
			goto end;
		}

		result = CYMB_ERROR_INVALID_ARGUMENT;

		goto end;
	}

	CymbConstString optionArgument = {};

	if(!option->argument && equal)
	{
		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNEXPECTED_ARGUMENT,
			.info = {
				.hint = {equal + 1, argument.length - optionString.length - 1}
			}
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
			goto end;
		}

		result = CYMB_ERROR_INVALID_ARGUMENT;

		goto end;
	}

	if(option->argument)
	{
		if(optionString.length == argument.length - 1)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_MISSING_ARGUMENT,
				.info = {
					.hint = argument
				}
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
				goto end;
			}

			result = CYMB_ERROR_INVALID_ARGUMENT;

			goto end;
		}

		if(equal)
		{
			optionArgument = (CymbConstString){equal + 1, argument.length - (equal - argument.string) - 1};
		}
		else
		{
			++parser->arguments;
			--parser->argumentCount;
			if(parser->argumentCount == 0)
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_MISSING_ARGUMENT,
					.info = {
						.hint = argument
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(parser->diagnostics, &diagnostic);

				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
					goto end;
				}

				result = CYMB_ERROR_INVALID_ARGUMENT;

				goto end;
			}

			optionArgument = *parser->arguments;
		}
	}

	result = cymbApplyOption(parser->options, option - longOptions, &optionArgument, parser->diagnostics);

	end:
	return result;
}

CymbResult cymbParseArguments(const CymbConstString* const arguments, const size_t argumentCount, CymbOptions* const options, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*options = (CymbOptions){
		.standard = CYMB_C23,
		.tabWidth = 8
	};

	CymbArgumentsParser parser = {
		.arguments = arguments,
		.argumentCount = argumentCount,
		.options = options,
		.diagnostics = diagnostics
	};

	bool separator = false;
	bool tooManyInputs = false;
	size_t inputsCapacity = 8;
	options->inputs = malloc(inputsCapacity * sizeof(options->inputs[0]));
	if(!options->inputs)
	{
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto end;
	}

	while(parser.argumentCount > 0)
	{
		// Input.
		if(separator || parser.arguments->string[0] != '-')
		{
			if(tooManyInputs)
			{
				goto next;
			}

			if(options->inputCount == inputsCapacity)
			{
				if(inputsCapacity * sizeof(options->inputs[0]) == cymbSizeMax)
				{
					free(options->inputs);
					tooManyInputs = true;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_TOO_MANY_INPUTS,
						.info = {
							.hint = parser.arguments[0]
						}
					};
					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
						goto end;
					}

					goto next;
				}

				const size_t newCapacity = inputsCapacity > cymbSizeMax / 2 ? cymbSizeMax : inputsCapacity * 2;
				const char** newInputs = realloc(options->inputs, newCapacity);
				if(!newInputs)
				{
					free(options->inputs);
					result = CYMB_ERROR_OUT_OF_MEMORY;
					goto end;
				}
				options->inputs = newInputs;
			}

			options->inputs[options->inputCount] = parser.arguments->string;
			++options->inputCount;

			goto next;
		}

		// Short option.
		if(parser.arguments->string[1] != '-')
		{
			const CymbResult shortResult = cymbParseShortOptions(&parser);
			if(shortResult != CYMB_SUCCESS)
			{
				result = shortResult;
			}
			if(result == CYMB_ERROR_OUT_OF_MEMORY)
			{
				goto end;
			}

			goto next;
		}

		// Long option.
		if(parser.arguments->string[2] == '\0')
		{
			separator = true;
			goto next;
		}

		const CymbResult longResult = cymbParseLongOption(&parser);
		if(longResult != CYMB_SUCCESS)
		{
			result = longResult;
		}
		if(result == CYMB_ERROR_OUT_OF_MEMORY)
		{
			goto end;
		}

		next:
		if(parser.argumentCount > 0)
		{
			++parser.arguments;
			--parser.argumentCount;
		}
	}

	if(options->inputCount == 0)
	{
		free(options->inputs);

		if(!options->help && !options->version)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_MISSING_ARGUMENT
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}
		}
	}

	end:
	return result;
}
