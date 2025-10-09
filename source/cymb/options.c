#include "cymb/options.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"
#include "cymb/version.h"

/*
 * An option.
 */
typedef enum CymbOption
{
	CYMB_OPTION_DEBUG,
	CYMB_OPTION_HELP,
	CYMB_OPTION_OUTPUT,
	CYMB_OPTION_STANDARD,
	CYMB_OPTION_TAB_WIDTH,
	CYMB_OPTION_VERSION
} CymbOption;

/*
 * A long option mapping.
 *
 * Fields:
 * - name: The option name.
 * - argument: A flag indicating if the options takes an argument.
 */
typedef struct CymbLongOption
{
	CymbConstString name;
	bool argument;
} CymbLongOption;

// The long options must be stored in the same order as the options enum and in alphabetical order.
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

/*
 * A short option mapping.
 *
 * Fields:
 * - name: The option name.
 * - longOptionIndex: The index of the corresponding long option.
 */
typedef struct CymbShortOption
{
	char name;
	size_t longOptionIndex;
} CymbShortOption;

// The short options must be stored in alphabetical order.
const CymbShortOption shortOptions[] = {
	{'g', CYMB_OPTION_DEBUG},
	{'h', CYMB_OPTION_HELP},
	{'o', CYMB_OPTION_OUTPUT},
	{'v', CYMB_OPTION_VERSION}
};
constexpr size_t shortOptionCount = CYMB_LENGTH(shortOptions);
constexpr size_t shortOptionSize = sizeof(shortOptions[0]);

/*
 * Check for short option equality.
 *
 * Parameters:
 * - characterVoid: A character.
 * - optionVoid: A short option.
 *
 * Returns:
 * - 0 if the options are equal.
 * - A negative value if the character comes before the option.
 * - A positive value if the character comes after the option.
 */
static int cymbCompareShortOptions(const void* const characterVoid, const void* const optionVoid)
{
	const char character = *(const char*)characterVoid;
	const CymbShortOption* const option = optionVoid;

	return (character > option->name) - (character < option->name);
}

/*
 * Check for long option equality.
 *
 * Parameters:
 * - stringVoid: A string.
 * - optionVoid: A long option.
 *
 * Returns:
 * - 0 if the options are equal.
 * - A negative value if the string comes before the option.
 * - A positive value if the string comes after the option.
 */
static int cymbCompareLongOptions(const void* const stringVoid, const void* const optionVoid)
{
	const CymbConstString* const string = stringVoid;
	const CymbLongOption* const option = optionVoid;

	const int result = strncmp(string->string, option->name.string, string->length);
	return result == 0 ? (string->length > option->name.length) - (string->length < option->name.length) : result;
}

/*
 * Apply an option.
 *
 * Parameters:
 * - options: The options.
 * - option: The option to apply.
 * - argument: The option argument.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if the argument is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
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
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

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
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				break;
			}

			break;

		case CYMB_OPTION_TAB_WIDTH:
			if(!isdigit((unsigned char)*argument->string))
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				break;
			}

			char* digitsEnd;
			const unsigned long tabWidth = strtoul(argument->string, &digitsEnd, 10);

			if(tabWidth == 0 || tabWidth > 16 || digitsEnd == argument->string || *digitsEnd != '\0')
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_INVALID_ARGUMENT,
					.info = {
						.hint = *argument
					}
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				break;
			}

			options->tabWidth = tabWidth;

			break;

		default:
			unreachable();
	}

	return result;
}

/*
 * An argument parser.
 *
 * Fields:
 * - arguments: The list of arguments.
 * - argumentCount: The number of arguments.
 * - options: The options.
 * - diagnostics: A list of diagnostics.
 */
typedef struct CymbArgumentsParser
{
	const CymbConstString* arguments;
	size_t argumentCount;

	CymbOptions* options;
	CymbDiagnosticList* diagnostics;
} CymbArgumentsParser;

/*
 * Parse short options.
 *
 * Parameters:
 * - parser: The parser.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CMB_INVALID if an option is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
static CymbResult cymbParseShortOptions(CymbArgumentsParser* const parser)
{
	CymbResult result = CYMB_SUCCESS;

	const char* argument = parser->arguments->string + 1;

	while(*argument != '\0')
	{
		const CymbShortOption* const shortOption = bsearch(argument, shortOptions, shortOptionCount, shortOptionSize, cymbCompareShortOptions);
		if(!shortOption)
		{
			result = CYMB_INVALID;

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
				result = CYMB_INVALID;

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

				continue;
			}

			optionArgument = *parser->arguments;
		}

		apply:
		const CymbResult applyResult = cymbApplyOption(parser->options, option - longOptions, &optionArgument, parser->diagnostics);
		if(applyResult == CYMB_OUT_OF_MEMORY)
		{
			goto end;
		}
		else if(applyResult == CYMB_INVALID)
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

/*
 * Parse a long option.
 *
 * Parameters:
 * - parser: The parser.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CMB_INVALID if an option is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
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
		result = CYMB_INVALID;

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
		}

		goto end;
	}

	CymbConstString optionArgument = {};

	if(!option->argument && equal)
	{
		result = CYMB_INVALID;

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
		}

		goto end;
	}

	if(option->argument)
	{
		if(optionString.length == argument.length - 1)
		{
			result = CYMB_INVALID;

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
			}

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
				result = CYMB_INVALID;

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
				}

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
		result = CYMB_OUT_OF_MEMORY;
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
				if(inputsCapacity >= cymbSizeMax / sizeof(options->inputs[0]))
				{
					CYMB_FREE(options->inputs);
					options->inputCount = 0;
					tooManyInputs = true;

					result = CYMB_OUT_OF_MEMORY;

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

				const size_t newCapacity = inputsCapacity > cymbSizeMax / 2 / sizeof(options->inputs[0]) ? cymbSizeMax / sizeof(options->inputs[0]) : inputsCapacity * 2;
				const char** newInputs = realloc(options->inputs, newCapacity);
				if(!newInputs)
				{
					CYMB_FREE(options->inputs);
					options->inputCount = 0;
					result = CYMB_OUT_OF_MEMORY;
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
			if(result == CYMB_OUT_OF_MEMORY)
			{
				CYMB_FREE(options->inputs);
				options->inputCount = 0;
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
		if(result == CYMB_OUT_OF_MEMORY)
		{
			CYMB_FREE(options->inputs);
			options->inputCount = 0;
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
		CYMB_FREE(options->inputs);

		if(!options->help && !options->version)
		{
			result = CYMB_INVALID;

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
