#include "cymb/lex.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

typedef struct CymbMapping
{
	CymbConstString string;
	CymbTokenType token;
} CymbMapping;

// Keywords must be stored from longest to shortest and in alphabetical order for bsearch.
// They could also be stored from shortest to longest, longer keywords would not be missed (e.g. "do" and "double") because the length of the identifier is known.
static const CymbMapping keywordMap[] = {
	{CYMB_STRING("_Static_assert"), CYMB_TOKEN__STATIC_ASSERT},
	{CYMB_STRING("static_assert"), CYMB_TOKEN_STATIC_ASSERT},
	{CYMB_STRING("typeof_unqual"), CYMB_TOKEN_TYPEOF_UNQUAL},
	{CYMB_STRING("_Thread_local"), CYMB_TOKEN__THREAD_LOCAL},
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
constexpr size_t keywordCount = CYMB_LENGTH(keywordMap);

// Tokens must be stored from longest to shortest to avoid missing longer tokens (e.g. "+" and "=" instead of "+=").
static const CymbMapping tokens[] = {
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
constexpr size_t tokenCount = CYMB_LENGTH(tokens);
constexpr size_t tokenSize = sizeof(tokens[0]);

#define CYMB_TOKEN_CASE(name) \
	case CYMB_TOKEN_##name: \
		return #name;

const char* cymbTokenTypeString(const CymbTokenType type)
{
	switch(type)
	{
		CYMB_TOKEN(CYMB_TOKEN_CASE)

		default:
			unreachable();
	}
}

bool cymbIsKeyword(const CymbTokenType type)
{
	return type >= CYMB_TOKEN_VOID && type <= CYMB_TOKEN_GOTO;
}

/*
 * Check for token string equality.
 *
 * Parameters:
 * - stringVoid: A string.
 * - tokenVoid: A pointer to a CymbConstString which is the token string.
 *
 * Returns:
 * - 0 if the strings are equal.
 * - 1 otherwise.
 */
static int cymbCompareToken(const void* const stringVoid, const void* const tokenVoid)
{
	const char* const string = stringVoid;
	const CymbConstString* const token = tokenVoid;

	return strncmp(token->string, string, token->length);
}

/*
 * Check for two strings equality.
 *
 * Parameters:
 * - stringVoid: A pointer to a CymbConstString to test.
 * - keywordVoid: A pointer to a keyword CymbConstString.
 *
 * Returns:
 * - 0 if the strings are equal.
 * - -1 if the string is before the keyword.
 * - 1 if the string is after the keyword.
 */
static int cymbCompareString(const void* const stringVoid, const void* const keywordVoid)
{
	const CymbConstString* const string = stringVoid;
	const CymbConstString* const keyword = keywordVoid;

	if(string->length != keyword->length)
	{
		return (string->length < keyword->length) - (string->length > keyword->length);
	}

	return strncmp(string->string, keyword->string, string->length);
}

CymbResult cymbParseString(CymbReader* const reader, CymbParseResult* const parseResult, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	if(*reader->string != '"')
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return CYMB_SUCCESS;
	}

	*parseResult = CYMB_PARSE_MATCH;

	token->type = CYMB_TOKEN_STRING;
	token->info.position = reader->position;
	token->info.line = reader->line;
	token->info.hint.string = reader->string;

	cymbReaderPop(reader);

	while(*reader->string != '\0' && *reader->string != '\n' && (*reader->string != '"' || *(reader->string - 1) == '\\'))
	{
		if(!isprint((unsigned char)*reader->string))
		{
			*parseResult = CYMB_PARSE_INVALID;
			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_STRING_CHARACTER,
				.info = {
					.position = reader->position,
					.line = reader->line,
					.hint = {reader->string, 1}
				}
			};
			const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(result != CYMB_SUCCESS)
			{
				return result;
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
		*parseResult = CYMB_PARSE_INVALID;
		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNFINISHED_STRING,
			.info = token->info
		};
		const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
	}

	return CYMB_SUCCESS;
}

CymbResult cymbParseCharacter(CymbReader* const reader, CymbParseResult* const parseResult, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	if(*reader->string != '\'')
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return CYMB_SUCCESS;
	}

	*parseResult = CYMB_PARSE_MATCH;

	token->type = CYMB_TOKEN_CONSTANT;
	token->constant.type = CYMB_CONSTANT_INT;
	token->info.position = reader->position;
	token->info.line = reader->line;
	token->info.hint.string = reader->string;
	token->info.hint.length = 3;

	cymbReaderPop(reader);

	if(
		reader->string[0] == '\0' ||
		reader->string[0] == '\n' ||
		reader->string[0] == '\'' ||
		reader->string[1] != '\''
	)
	{
		*parseResult = CYMB_PARSE_INVALID;

		if(reader->string[0] == '\0' || reader->string[0] == '\n')
		{
			token->info.hint.length = 1;
		}
		else
		{
			token->info.hint.length = 2;
		}

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_CHARACTER_CONSTANT,
			.info = token->info
		};
		const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
	}

	if(reader->string[0] == '\0' || reader->string[0] == '\n')
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

	return CYMB_SUCCESS;
}

CymbResult cymbParseToken(CymbReader* const reader, CymbParseResult* const parseResult, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	(void)diagnostics;

	const CymbMapping* const mapping = cymbFind(reader->string, tokens, tokenCount, tokenSize, cymbCompareToken);
	if(!mapping)
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return CYMB_SUCCESS;
	}

	token->type = mapping->token;
	token->info.position = reader->position;
	token->info.line = reader->line;
	token->info.hint.string = reader->string;
	token->info.hint.length = mapping->string.length;

	cymbReaderSkip(reader, token->info.hint.length);

	*parseResult = CYMB_PARSE_MATCH;
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
	const char upperCharacter = toupper((unsigned char)character);

	if(upperCharacter >= '0' && upperCharacter <= '9')
	{
		return upperCharacter - '0' < base;
	}

	return base == 16 && upperCharacter >= 'A' && upperCharacter <= 'F';
}

CymbResult cymbParseConstant(CymbReader* const reader, CymbParseResult* const parseResult, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	if(!isdigit((unsigned char)*reader->string))
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return CYMB_SUCCESS;
	}

	*parseResult = CYMB_PARSE_MATCH;

	token->type = CYMB_TOKEN_CONSTANT;
	token->info.position = reader->position;
	token->info.line = reader->line;
	token->info.hint.string = reader->string;

	const char* const start = reader->string;

	// Parse base.
	unsigned char base = 10;
	if(reader->string[0] == '0')
	{
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
			const char* separators = reader->string + 2;
			while(*separators == '\'')
			{
				++separators;
			}

			base = cymbIsDigit(*separators, base) ? base : 8;
		}

		cymbReaderSkip(reader, 2 * (base != 8));
	}

	const char* end = reader->string;
	while(cymbIsDigit(*end, base) || *end == '\'')
	{
		++end;
	}
	while(isalnum((unsigned char)*end) || *end == '_')
	{
		++end;
	}
	token->info.hint.length = end - start;

	// Parse value.
	bool previousSeparator = false;
	token->constant.value = 0;
	if(*reader->string == '\'')
	{
		const CymbDiagnostic diagnostic = {
			.type = CYMB_SEPARATOR_AFTER_BASE,
			.info = {
				.position = reader->position,
				.line = reader->line,
				.hint = {reader->string, 1}
			}
		};
		const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		cymbReaderPop(reader);
		previousSeparator = true;
	}
	while(cymbIsDigit(*reader->string, base) || *reader->string == '\'')
	{
		if(*reader->string == '\'')
		{
			if(previousSeparator)
			{
				const CymbDiagnostic diagnostic = {
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = reader->position,
						.line = reader->line,
						.hint = {reader->string, 1}
					}
				};
				const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(result != CYMB_SUCCESS)
				{
					return result;
				}
			}

			previousSeparator = true;
			goto next;
		}

		previousSeparator = false;

		const unsigned char digit = *reader->string - (*reader->string <= '9' ? '0' : 'A');

		if(token->constant.value > (ULLONG_MAX - digit) / base)
		{
			*parseResult = CYMB_PARSE_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_CONSTANT_TOO_LARGE,
				.info = token->info
			};
			const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(result != CYMB_SUCCESS)
			{
				return result;
			}

			break;
		}

		token->constant.value = token->constant.value * base + digit;

		next:
		cymbReaderPop(reader);
	}

	if(previousSeparator)
	{
		*parseResult = CYMB_PARSE_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_TRAILING_SEPARATOR,
			.info = {
				.position = {reader->position.line, reader->position.column - 1},
				.line = reader->line,
				.hint = {reader->string - 1, 1}
			}
		};
		const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
	}

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
				*parseResult = CYMB_PARSE_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(result != CYMB_SUCCESS)
				{
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
				*parseResult = CYMB_PARSE_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(result != CYMB_SUCCESS)
				{
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
				*parseResult = CYMB_PARSE_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_CONSTANT_TOO_LARGE,
					.info = token->info
				};
				const CymbResult result = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(result != CYMB_SUCCESS)
				{
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
			*parseResult = CYMB_PARSE_INVALID;

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

	return CYMB_SUCCESS;
}

CymbResult cymbParseIdentifier(CymbReader* const reader, CymbParseResult* const parseResult, CymbToken* const token, CymbDiagnosticList* const diagnostics)
{
	(void)diagnostics;

	if(!isalpha((unsigned char)*reader->string) && *reader->string != '_')
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return CYMB_SUCCESS;
	}

	*parseResult = CYMB_PARSE_MATCH;

	token->type = CYMB_TOKEN_IDENTIFIER;
	token->info.position = reader->position;
	token->info.line = reader->line;
	token->info.hint.string = reader->string;

	cymbReaderPop(reader);
	while(isalnum((unsigned char)*reader->string) || *reader->string == '_')
	{
		cymbReaderPop(reader);
	}

	token->info.hint.length = reader->string - token->info.hint.string;

	const CymbMapping* const keyword = bsearch(&token->info.hint, keywordMap, keywordCount, sizeof(keywordMap[0]), cymbCompareString);
	if(keyword)
	{
		token->type = keyword->token;
	}

	return CYMB_SUCCESS;
}

CymbResult cymbLex(const char* const string, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	bool valid = true;

	tokens->count = 0;

	if(*string == '\0')
	{
		tokens->tokens = nullptr;

		goto end;
	}

	size_t tokensCapacity = 1024;
	tokens->tokens = malloc(tokensCapacity * sizeof(tokens->tokens[0]));
	if(!tokens->tokens)
	{
		result = CYMB_ERROR_OUT_OF_MEMORY;
		return result;
	}

	CymbReader reader;
	cymbReaderCreate(string, diagnostics->tabWidth, &reader);

	static const CymbLexFunction lexFunctions[] = {
		cymbParseString,
		cymbParseCharacter,
		cymbParseToken,
		cymbParseConstant,
		cymbParseIdentifier
	};
	static constexpr size_t lexFunctionCount = CYMB_LENGTH(lexFunctions);

	CymbParseResult parseResult;
	while(*reader.string != '\0')
	{
		cymbReaderSkipSpaces(&reader);
		if(*reader.string == '\0')
		{
			break;
		}

		if(tokens->count == tokensCapacity)
		{
			if(tokensCapacity * sizeof(tokens->tokens) == cymbSizeMax)
			{
				free(tokens->tokens);
				return CYMB_ERROR_OUT_OF_MEMORY;
			}

			tokensCapacity = tokensCapacity * sizeof(tokens->tokens[0]) > cymbSizeMax / 2 ? cymbSizeMax : tokensCapacity * 2;
			CymbToken* const newTokens = realloc(tokens->tokens, tokensCapacity);
			if(!newTokens)
			{
				free(tokens->tokens);
				return CYMB_ERROR_OUT_OF_MEMORY;
			}
			tokens->tokens = newTokens;
		}

		tokens->tokens[tokens->count].info.hint.string = reader.string;

		for(size_t lexFunctionIndex = 0; lexFunctionIndex < lexFunctionCount; ++lexFunctionIndex)
		{
			result = lexFunctions[lexFunctionIndex](&reader, &parseResult, &tokens->tokens[tokens->count], diagnostics);
			if(result != CYMB_SUCCESS)
			{
				goto clear;
			}
			if(parseResult == CYMB_PARSE_MATCH)
			{
				goto append;
			}
			else if(parseResult == CYMB_PARSE_INVALID)
			{
				goto append_invalid;
			}
		}

		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNKNOWN_TOKEN,
			.info = {
				.position = reader.position,
				.line = reader.line,
				.hint = {reader.string, 1}
			}
		};
		result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(result != CYMB_SUCCESS)
		{
			goto clear;
		}

		cymbReaderPop(&reader);
		continue;

		append_invalid:
		valid = false;

		append:
		++tokens->count;
	}

	if(tokens->count == 0)
	{
		free(tokens->tokens);
		tokens->tokens = nullptr;
	}
	else if(tokens->count < tokensCapacity)
	{
		CymbToken* const newTokens = realloc(tokens->tokens, tokens->count * sizeof(tokens->tokens[0]));
		if(!newTokens)
		{
			free(tokens->tokens);
			return CYMB_ERROR_UNKNOWN;
		}
		tokens->tokens = newTokens;
	}
	goto end;

	clear:
	free(tokens->tokens);

	end:
	if(result == CYMB_SUCCESS)
	{
		return valid ? result : CYMB_ERROR_INVALID_ARGUMENT;
	}
	return result;
}

void cymbFreeTokenList(CymbTokenList* const tokens)
{
	CYMB_FREE(tokens->tokens);
	tokens->count = 0;
}
