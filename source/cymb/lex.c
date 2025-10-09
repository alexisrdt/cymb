#include "cymb/lex.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

/*
 * A mapping from string to token type.
 *
 * Fields:
 * - string: A string.
 * - token: A token type.
 */
typedef struct CymbMapping
{
	CymbConstString string;
	CymbTokenType token;
} CymbMapping;

// Keywords must be stored from longest to shortest and in alphabetical order for bsearch.
// They could also be stored from shortest to longest, longer keywords would not be missed (e.g. "do" and "double") because the length of the identifier is known.
static const CymbMapping keywords[] = {
	{CYMB_STRING("_Static_assert"), CYMB_TOKEN__STATIC_ASSERT},
	{CYMB_STRING("_Thread_local"), CYMB_TOKEN__THREAD_LOCAL},
	{CYMB_STRING("static_assert"), CYMB_TOKEN_STATIC_ASSERT},
	{CYMB_STRING("typeof_unqual"), CYMB_TOKEN_TYPEOF_UNQUAL},
	{CYMB_STRING("thread_local"), CYMB_TOKEN_THREAD_LOCAL},
	{CYMB_STRING("_Decimal128"), CYMB_TOKEN__DECIMAL_128},
	{CYMB_STRING("_Decimal32"), CYMB_TOKEN__DECIMAL_32},
	{CYMB_STRING("_Decimal64"), CYMB_TOKEN__DECIMAL_64},
	{CYMB_STRING("_Imaginary"), CYMB_TOKEN__IMAGINARY},
	{CYMB_STRING("constexpr"), CYMB_TOKEN_CONSTEXPR},
	{CYMB_STRING("_Alignas"), CYMB_TOKEN__ALIGNAS},
	{CYMB_STRING("_Alignof"), CYMB_TOKEN__ALIGNOF},
	{CYMB_STRING("_Complex"), CYMB_TOKEN__COMPLEX},
	{CYMB_STRING("_Generic"), CYMB_TOKEN__GENERIC},
	{CYMB_STRING("continue"), CYMB_TOKEN_CONTINUE},
	{CYMB_STRING("register"), CYMB_TOKEN_REGISTER},
	{CYMB_STRING("restrict"), CYMB_TOKEN_RESTRICT},
	{CYMB_STRING("unsigned"), CYMB_TOKEN_UNSIGNED},
	{CYMB_STRING("volatile"), CYMB_TOKEN_VOLATILE},
	{CYMB_STRING("_Atomic"), CYMB_TOKEN__ATOMIC},
	{CYMB_STRING("_BitInt"), CYMB_TOKEN__BIT_INT},
	{CYMB_STRING("alignas"), CYMB_TOKEN_ALIGNAS},
	{CYMB_STRING("alignof"), CYMB_TOKEN_ALIGNOF},
	{CYMB_STRING("default"), CYMB_TOKEN_DEFAULT},
	{CYMB_STRING("typedef"), CYMB_TOKEN_TYPEDEF},
	{CYMB_STRING("double"), CYMB_TOKEN_DOUBLE},
	{CYMB_STRING("extern"), CYMB_TOKEN_EXTERN},
	{CYMB_STRING("return"), CYMB_TOKEN_RETURN},
	{CYMB_STRING("signed"), CYMB_TOKEN_SIGNED},
	{CYMB_STRING("sizeof"), CYMB_TOKEN_SIZEOF},
	{CYMB_STRING("static"), CYMB_TOKEN_STATIC},
	{CYMB_STRING("struct"), CYMB_TOKEN_STRUCT},
	{CYMB_STRING("switch"), CYMB_TOKEN_SWITCH},
	{CYMB_STRING("typeof"), CYMB_TOKEN_TYPEOF},
	{CYMB_STRING("_Bool"), CYMB_TOKEN__BOOL},
	{CYMB_STRING("break"), CYMB_TOKEN_BREAK},
	{CYMB_STRING("const"), CYMB_TOKEN_CONST},
	{CYMB_STRING("false"), CYMB_TOKEN_FALSE},
	{CYMB_STRING("float"), CYMB_TOKEN_FLOAT},
	{CYMB_STRING("short"), CYMB_TOKEN_SHORT},
	{CYMB_STRING("union"), CYMB_TOKEN_UNION},
	{CYMB_STRING("while"), CYMB_TOKEN_WHILE},
	{CYMB_STRING("auto"), CYMB_TOKEN_AUTO},
	{CYMB_STRING("bool"), CYMB_TOKEN_BOOL},
	{CYMB_STRING("case"), CYMB_TOKEN_CASE},
	{CYMB_STRING("char"), CYMB_TOKEN_CHAR},
	{CYMB_STRING("else"), CYMB_TOKEN_ELSE},
	{CYMB_STRING("enum"), CYMB_TOKEN_ENUM},
	{CYMB_STRING("goto"), CYMB_TOKEN_GOTO},
	{CYMB_STRING("long"), CYMB_TOKEN_LONG},
	{CYMB_STRING("true"), CYMB_TOKEN_TRUE},
	{CYMB_STRING("void"), CYMB_TOKEN_VOID},
	{CYMB_STRING("for"), CYMB_TOKEN_FOR},
	{CYMB_STRING("int"), CYMB_TOKEN_INT},
	{CYMB_STRING("do"), CYMB_TOKEN_DO},
	{CYMB_STRING("if"), CYMB_TOKEN_IF}
};
constexpr size_t keywordCount = CYMB_LENGTH(keywords);

// Punctuators must be stored from longest to shortest to avoid missing longer ones (e.g. "+" and "=" instead of "+=").
static const CymbMapping punctuators[] = {
	{CYMB_STRING("<<="), CYMB_TOKEN_LEFT_SHIFT_EQUAL},
	{CYMB_STRING(">>="), CYMB_TOKEN_RIGHT_SHIFT_EQUAL},
	{CYMB_STRING("+="), CYMB_TOKEN_PLUS_EQUAL},
	{CYMB_STRING("-="), CYMB_TOKEN_MINUS_EQUAL},
	{CYMB_STRING("*="), CYMB_TOKEN_STAR_EQUAL},
	{CYMB_STRING("/="), CYMB_TOKEN_SLASH_EQUAL},
	{CYMB_STRING("%="), CYMB_TOKEN_PERCENT_EQUAL},
	{CYMB_STRING("&="), CYMB_TOKEN_AMPERSAND_EQUAL},
	{CYMB_STRING("|="), CYMB_TOKEN_BAR_EQUAL},
	{CYMB_STRING("^="), CYMB_TOKEN_CARET_EQUAL},
	{CYMB_STRING("<<"), CYMB_TOKEN_LEFT_SHIFT},
	{CYMB_STRING(">>"), CYMB_TOKEN_RIGHT_SHIFT},
	{CYMB_STRING("++"), CYMB_TOKEN_PLUS_PLUS},
	{CYMB_STRING("--"), CYMB_TOKEN_MINUS_MINUS},
	{CYMB_STRING("=="), CYMB_TOKEN_EQUAL_EQUAL},
	{CYMB_STRING("!="), CYMB_TOKEN_NOT_EQUAL},
	{CYMB_STRING("<="), CYMB_TOKEN_LESS_EQUAL},
	{CYMB_STRING(">="), CYMB_TOKEN_GREATER_EQUAL},
	{CYMB_STRING("&&"), CYMB_TOKEN_AMPERSAND_AMPERSAND},
	{CYMB_STRING("||"), CYMB_TOKEN_BAR_BAR},
	{CYMB_STRING("->"), CYMB_TOKEN_ARROW},
	{CYMB_STRING("("), CYMB_TOKEN_OPEN_PARENTHESIS},
	{CYMB_STRING(")"), CYMB_TOKEN_CLOSE_PARENTHESIS},
	{CYMB_STRING("{"), CYMB_TOKEN_OPEN_BRACE},
	{CYMB_STRING("}"), CYMB_TOKEN_CLOSE_BRACE},
	{CYMB_STRING("["), CYMB_TOKEN_OPEN_BRACKET},
	{CYMB_STRING("]"), CYMB_TOKEN_CLOSE_BRACKET},
	{CYMB_STRING(";"), CYMB_TOKEN_SEMICOLON},
	{CYMB_STRING("+"), CYMB_TOKEN_PLUS},
	{CYMB_STRING("-"), CYMB_TOKEN_MINUS},
	{CYMB_STRING("*"), CYMB_TOKEN_STAR},
	{CYMB_STRING("/"), CYMB_TOKEN_SLASH},
	{CYMB_STRING("%"), CYMB_TOKEN_PERCENT},
	{CYMB_STRING("&"), CYMB_TOKEN_AMPERSAND},
	{CYMB_STRING("|"), CYMB_TOKEN_BAR},
	{CYMB_STRING("^"), CYMB_TOKEN_CARET},
	{CYMB_STRING("~"), CYMB_TOKEN_TILDE},
	{CYMB_STRING("="), CYMB_TOKEN_EQUAL},
	{CYMB_STRING("<"), CYMB_TOKEN_LESS},
	{CYMB_STRING(">"), CYMB_TOKEN_GREATER},
	{CYMB_STRING("!"), CYMB_TOKEN_EXCLAMATION},
	{CYMB_STRING("?"), CYMB_TOKEN_QUESTION},
	{CYMB_STRING(","), CYMB_TOKEN_COMMA},
	{CYMB_STRING("."), CYMB_TOKEN_DOT},
	{CYMB_STRING(":"), CYMB_TOKEN_COLON}
};
constexpr size_t punctuatorCount = CYMB_LENGTH(punctuators);
constexpr size_t punctuatorSize = sizeof(punctuators[0]);

bool cymbIsKeyword(const CymbTokenType type)
{
	return type >= CYMB_TOKEN_VOID && type <= CYMB_TOKEN_GOTO;
}

/*
 * Check for punctuator equality.
 *
 * Parameters:
 * - stringVoid: A string.
 * - punctuatorVoid: A punctuator string.
 *
 * Returns:
 * - 0 if the strings are equal.
 * - A negative value if the first string comes before the second one.
 * - A positive value if the first string comes after the second one.
 */
static int cymbComparePunctuator(const void* const stringVoid, const void* const punctuatorVoid)
{
	const char* const string = stringVoid;
	const CymbConstString* const punctuator = punctuatorVoid;

	return strncmp(punctuator->string, string, punctuator->length);
}

/*
 * Check for two strings equality.
 *
 * Parameters:
 * - stringVoid: A string to test.
 * - keywordVoid: A keyword mapping.
 *
 * Returns:
 * - 0 if the strings are equal.
 * - A negative value if the first string comes before the second one.
 * - A positive value if the first string comes after the second one.
 */
static int cymbCompareString(const void* const stringVoid, const void* const keywordVoid)
{
	const CymbConstString* const string = stringVoid;
	const CymbMapping* const keyword = keywordVoid;

	if(string->length != keyword->string.length)
	{
		return (string->length < keyword->string.length) - (string->length > keyword->string.length);
	}

	return strncmp(string->string, keyword->string.string, string->length);
}

CymbResult cymbParseString(CymbReader* const reader, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	if(*reader->string != '"')
	{
		result = CYMB_NO_MATCH;
		goto end;
	}

	token->type = CYMB_TOKEN_STRING;

	cymbReaderPop(reader);

	while(*reader->string != '\0' && *reader->string != '\n' && (*reader->string != '"' || *(reader->string - 1) == '\\'))
	{
		if(!isprint((unsigned char)*reader->string))
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_STRING_CHARACTER,
				.info = {
					.position = reader->position,
					.line = reader->line,
					.hint = {reader->string, 1}
				}
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
				goto end;
			}
		}

		cymbReaderPop(reader);
	}

	token->info.hint.length = reader->string - token->info.hint.string;

	if(*reader->string == '"')
	{
		cymbReaderPop(reader);
		++token->info.hint.length;
	}
	else
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNFINISHED_STRING,
			.info = token->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}
	}

	end:
	return result;
}

CymbResult cymbParseCharacter(CymbReader* const reader, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	if(*reader->string != '\'')
	{
		result = CYMB_NO_MATCH;
		goto end;
	}

	token->type = CYMB_TOKEN_CONSTANT;
	token->constant.type = CYMB_CONSTANT_INT;
	token->info.hint.length = 3;

	cymbReaderPop(reader);

	// Check if the character is not terminated.
	const bool endOfLine = reader->string[0] == '\0' || reader->string[0] == '\n';
	if(
		endOfLine ||
		reader->string[0] == '\'' ||
		reader->string[1] != '\''
	)
	{
		result = CYMB_INVALID;

		token->info.hint.length = 1 + !endOfLine;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_CHARACTER_CONSTANT,
			.info = token->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
			goto end;
		}
	}

	if(endOfLine)
	{
		token->constant.value = 0;
	}
	else if(reader->string[0] == '\'')
	{
		token->constant.value = 0;
		cymbReaderPop(reader);
	}
	else
	{
		token->constant.value = reader->string[0];
		cymbReaderSkip(reader, 1 + (reader->string[1] == '\''));
	}

	end:
	return result;
}

CymbResult cymbParsePunctuator(CymbReader* const reader, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	(void)diagnostics;

	const CymbMapping* const mapping = cymbFind(reader->string, punctuators, punctuatorCount, punctuatorSize, cymbComparePunctuator);
	if(!mapping)
	{
		return CYMB_NO_MATCH;
	}

	token->type = mapping->token;
	token->info.hint.length = mapping->string.length;

	cymbReaderSkip(reader, token->info.hint.length);

	return CYMB_SUCCESS;
}

/*
 * Check if a character is a digit in some base.
 *
 * Parameters:
 * - character: The character to test.
 * - base: The base, should be 2, 8, 10 or 16.
 *
 * Returns:
 * - true if the character is a digit in the given base.
 * - false otherwise.
 */
static bool cymbIsDigit(const char character, const unsigned char base)
{
	if(character >= '0' && character <= '9')
	{
		return character - '0' < base;
	}

	const char upperCharacter = toupper((unsigned char)character);
	return base == 16 && upperCharacter >= 'A' && upperCharacter <= 'F';
}

/*
 * Parse base prefix.
 *
 * Parameters:
 * - reader: A reader.
 *
 * Returns:
 * - The parsed base.
 */
static unsigned char cymbParseBase(CymbReader* const reader)
{
	unsigned char base = 10;

	if(reader->string[0] != '0')
	{
		goto end;
	}

	const char baseCharacter = tolower((unsigned char)reader->string[1]);
	switch(baseCharacter)
	{
		case 'x':
			base = 16;
			break;

		case 'b':
			base = 2;
			break;

		default:
			base = 8;
			break;
	}

	if(base != 8)
	{
		const char* separator = &reader->string[2];
		while(*separator == '\'')
		{
			++separator;
		}

		if(!cymbIsDigit(*separator, base))
		{
			base = 8;
		}
	}

	cymbReaderSkip(reader, 2 * (base != 8));

	end:
	return base;
}

CymbResult cymbParseUnsigned(CymbReader* const reader, uintmax_t* const value, unsigned char base, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*value = 0;

	if(base == 0 && !isdigit((unsigned char)*reader->string))
	{
		result = CYMB_NO_MATCH;
		goto end;
	}

	CymbDiagnosticInfo info = {
		.position = reader->position,
		.line = reader->line,
		.hint = {.string = reader->string}
	};

	// Parse base.
	if(base == 0)
	{
		base = cymbParseBase(reader);
	}

	const char* end = reader->string;
	while(cymbIsDigit(*end, base) || *end == '\'')
	{
		++end;
	}
	info.hint.length = end - info.hint.string;

	// Parse value.
	bool previousSeparator = false;
	if(*reader->string == '\'')
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_SEPARATOR_AFTER_BASE,
			.info = {
				.position = reader->position,
				.line = reader->line,
				.hint = {reader->string, 1}
			}
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
			goto end;
		}

		cymbReaderPop(reader);
		previousSeparator = true;
	}

	bool tooLarge = false;
	while(cymbIsDigit(*reader->string, base) || *reader->string == '\'')
	{
		if(*reader->string == '\'')
		{
			if(previousSeparator)
			{
				result = CYMB_INVALID;

				const size_t column = reader->position.column - 1;

				const char* const start = reader->string - 1;
				while(*reader->string == '\'')
				{
					cymbReaderPop(reader);
				}

				const CymbDiagnostic diagnostic = {
					.type = CYMB_DUPLICATE_SEPARATORS,
					.info = {
						.position = {reader->position.line, column},
						.line = reader->line,
						.hint = {start, reader->string - start}
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

			previousSeparator = true;
			cymbReaderPop(reader);

			goto next;
		}

		previousSeparator = false;

		const unsigned char digit = toupper((unsigned char)*reader->string) - (*reader->string <= '9' ? '0' : 'A');

		if(!tooLarge && *value > (UINTMAX_MAX - digit) / base)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_CONSTANT_TOO_LARGE,
				.info = info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
				goto end;
			}

			tooLarge = true;
		}

		*value = *value * base + digit;

		cymbReaderPop(reader);

		next:
	}

	if(previousSeparator)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_TRAILING_SEPARATOR,
			.info = {
				.position = {reader->position.line, reader->position.column - 1},
				.line = reader->line,
				.hint = {reader->string - 1, 1}
			}
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}
	}

	end:
	return result;
}

/*
 * Parse constant suffix.
 *
 * Parameters:
 * - reader: A reader.
 * - token: A token.
 * - base: The base of the ocnstant.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if the suffix is invalid or the constant too large.
 * - CYMB_OUT_OF_MEMORY if a diagnostic could not be added.
 */
static CymbResult cymbParseSuffix(CymbReader* const reader, CymbToken* const token, const unsigned char base, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	const char* end = reader->string;
	while(isalpha((unsigned char)*end))
	{
		++end;
	}
	token->info.hint.length = end - token->info.hint.string;

	const char suffix[] = {
		tolower(reader->string[0]),
		suffix[0] == '\0' ? '\0' : tolower(reader->string[1]),
		suffix[1] == '\0' ? '\0' : tolower(reader->string[2]),
	};
	if(
		((suffix[0] == 'u' && suffix[1] == 'l' && reader->string[2] == reader->string[1]) ||
		(suffix[0] == 'l' && reader->string[1] == reader->string[0] && suffix[2] == 'u')) &&
		!isalnum((unsigned char)reader->string[3]) && reader->string[3] != '_'
	)
	{
		token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
	}
	else if(
		((suffix[0] == 'u' && suffix[1] == 'l') ||
		(suffix[0] == 'l' && suffix[1] == 'u')) &&
		!isalnum((unsigned char)reader->string[2]) && reader->string[2] != '_'
	)
	{
		token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG;
		if(token->constant.value > ULONG_MAX)
		{
			token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
		}
	}
	else if(suffix[0] == 'u' && !isalnum((unsigned char)reader->string[1]) && reader->string[1] != '_')
	{
		token->constant.type = CYMB_CONSTANT_UNSIGNED_INT;
		if(token->constant.value > UINT_MAX)
		{
			token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG;
		}
		if(token->constant.value > ULONG_MAX)
		{
			token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
		}
	}
	else if(suffix[0] == 'l' && reader->string[1] == reader->string[0] && !isalnum((unsigned char)reader->string[2]) && reader->string[2] != '_')
	{
		token->constant.type = CYMB_CONSTANT_LONG_LONG;
		if(base == 10)
		{
			if(token->constant.value > LLONG_MAX)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
					return result;
				}
			}
		}
		else
		{
			if(token->constant.value > LLONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
			}
		}
	}
	else if(suffix[0] == 'l' && !isalnum((unsigned char)reader->string[1]) && reader->string[1] != '_')
	{
		token->constant.type = CYMB_CONSTANT_LONG;
		if(base == 10)
		{
			if(token->constant.value > LONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG_LONG;
			}
			if(token->constant.value > LLONG_MAX)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
					return result;
				}
			}
		}
		else
		{
			if(token->constant.value > LONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG;
			}
			if(token->constant.value > ULONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG_LONG;
			}
			if(token->constant.value > LLONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
			}
		}
	}
	else
	{
		token->constant.type = CYMB_CONSTANT_INT;
		if(base == 10)
		{
			if(token->constant.value > INT_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG;
			}
			if(token->constant.value > LONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG_LONG;
			}
			if(token->constant.value > LLONG_MAX)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
					return result;
				}
			}
		}
		else
		{
			if(token->constant.value > INT_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_INT;
			}
			if(token->constant.value > UINT_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG;
			}
			if(token->constant.value > LONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG;
			}
			if(token->constant.value > ULONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_LONG_LONG;
			}
			if(token->constant.value > LLONG_MAX)
			{
				token->constant.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG;
			}
		}

		if(isalnum((unsigned char)reader->string[0]) || reader->string[0] == '_')
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_CONSTANT_SUFFIX,
				.info = {
					.position = reader->position,
					.line = reader->line,
					.hint = {reader->string, end - reader->string}
				}
			};
			const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(result != CYMB_SUCCESS)
			{
				return result;
			}
		}
	}

	cymbReaderSkip(reader, end - reader->string);

	return result;
}

CymbResult cymbParseConstant(CymbReader* const reader, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	token->type = CYMB_TOKEN_CONSTANT;
	token->constant.type = CYMB_CONSTANT_INT;

	if(!isdigit((unsigned char)*reader->string))
	{
		return CYMB_NO_MATCH;
	}

	const CymbDiagnostic* diagnostic = diagnostics->end;

	const unsigned char base = cymbParseBase(reader);
	uintmax_t value;
	CymbResult result = cymbParseUnsigned(reader, &value, base, diagnostics);
	token->info.hint.length = reader->string - token->info.hint.string;
	token->constant.value = value;

	if(result != CYMB_SUCCESS && result != CYMB_INVALID)
	{
		return result;
	}

	bool tooLarge = false;
	if(result == CYMB_SUCCESS && value > ULLONG_MAX)
	{
		result = CYMB_INVALID;
		tooLarge = true;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_CONSTANT_TOO_LARGE,
			.info = token->info
		};
		result = cymbDiagnosticAdd(diagnostics, &diagnostic);

		return result;
	}
	token->constant.value = value;

	if(!tooLarge)
	{
		if(!diagnostic)
		{
			diagnostic = diagnostics->start;
		}

		while(diagnostic)
		{
			if(diagnostic->type == CYMB_CONSTANT_TOO_LARGE)
			{
				tooLarge = true;
				break;
			}

			diagnostic = diagnostic->next;
		}
	}
	
	if(tooLarge)
	{
		token->constant = (CymbConstant){CYMB_CONSTANT_INT, 0};
	}

	// Parse suffix.
	const CymbResult suffixResult = cymbParseSuffix(reader, token, base, diagnostics);
	if(result == CYMB_SUCCESS)
	{
		result = suffixResult;
	}

	return result;
}

CymbResult cymbParseIdentifier(CymbReader* const reader, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	(void)diagnostics;

	CymbResult result = CYMB_SUCCESS;

	if(!isalpha((unsigned char)*reader->string) && *reader->string != '_')
	{
		result = CYMB_NO_MATCH;
		return result;
	}

	token->type = CYMB_TOKEN_IDENTIFIER;

	cymbReaderPop(reader);
	while(isalnum((unsigned char)*reader->string) || *reader->string == '_')
	{
		cymbReaderPop(reader);
	}

	token->info.hint.length = reader->string - token->info.hint.string;

	const CymbMapping* const keyword = bsearch(&token->info.hint, keywords, keywordCount, sizeof(keywords[0]), cymbCompareString);
	if(keyword)
	{
		token->type = keyword->token;
	}

	return result;
}

CymbResult cymbLex(const char* const string, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	tokens->count = 0;

	size_t tokensCapacity = 1024;
	tokens->tokens = malloc(tokensCapacity * sizeof(tokens->tokens[0]));
	if(!tokens->tokens)
	{
		result = CYMB_OUT_OF_MEMORY;
		return result;
	}

	CymbReader reader;
	cymbReaderCreate(string, diagnostics->tabWidth, &reader);

	static const CymbLexFunction lexFunctions[] = {
		cymbParseString,
		cymbParseCharacter,
		cymbParsePunctuator,
		cymbParseConstant,
		cymbParseIdentifier
	};
	static constexpr size_t lexFunctionCount = CYMB_LENGTH(lexFunctions);

	while(cymbReaderSkipSpaces(&reader), *reader.string != '\0')
	{
		if(tokens->count == tokensCapacity)
		{
			if(tokensCapacity * sizeof(tokens->tokens) == cymbSizeMax)
			{
				result = CYMB_OUT_OF_MEMORY;
				goto clear;
			}

			tokensCapacity = tokensCapacity > cymbSizeMax / 2 / sizeof(tokens->tokens[0]) ? cymbSizeMax / sizeof(tokens->tokens[0]) : tokensCapacity * 2;
			CymbToken* const newTokens = realloc(tokens->tokens, tokensCapacity * sizeof(tokens->tokens[0]));
			if(!newTokens)
			{
				result = CYMB_OUT_OF_MEMORY;
				goto clear;
			}
			tokens->tokens = newTokens;
		}

		CymbToken* const token = &tokens->tokens[tokens->count];
		*token = (CymbToken){
			.info = {
				.position = reader.position,
				.line = reader.line,
				.hint = {.string = reader.string}
			}
		};

		for(size_t lexFunctionIndex = 0; lexFunctionIndex < lexFunctionCount; ++lexFunctionIndex)
		{
			const CymbResult lexResult = lexFunctions[lexFunctionIndex](&reader, token, diagnostics);

			if(lexResult == CYMB_INVALID)
			{
				result = CYMB_INVALID;
			}

			if(lexResult == CYMB_SUCCESS || lexResult == CYMB_INVALID)
			{
				goto append;
			}
			if(lexResult != CYMB_NO_MATCH)
			{
				goto clear;
			}
		}

		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNKNOWN_TOKEN,
			.info = {
				.position = reader.position,
				.line = reader.line,
				.hint = {reader.string, 1}
			}
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
			goto clear;
		}

		cymbReaderPop(&reader);
		continue;

		append:
		++tokens->count;
	}

	if(tokens->count == 0)
	{
		goto clear;
	}

	if(tokens->count < tokensCapacity)
	{
		CymbToken* const newTokens = realloc(tokens->tokens, tokens->count * sizeof(tokens->tokens[0]));
		if(!newTokens)
		{
			result = CYMB_OUT_OF_MEMORY;
			goto clear;
		}
		tokens->tokens = newTokens;
	}
	goto end;

	clear:
	cymbFreeTokenList(tokens);

	end:
	return result;
}

void cymbFreeTokenList(CymbTokenList* const tokens)
{
	CYMB_FREE(tokens->tokens);
	*tokens = (CymbTokenList){};
}
