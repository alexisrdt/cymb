#ifndef CYMB_LEX_H
#define CYMB_LEX_H

#include <stddef.h>

#include "cymb/diagnostic.h"
#include "cymb/memory.h"
#include "cymb/reader.h"
#include "cymb/result.h"

/*
 * A token type.
 */
typedef enum CymbTokenType
{
	CYMB_TOKEN_IDENTIFIER,
	/* Keywords */
	CYMB_TOKEN_VOID,
	CYMB_TOKEN__BOOL,
	CYMB_TOKEN_BOOL,
	CYMB_TOKEN_FALSE,
	CYMB_TOKEN_TRUE,
	CYMB_TOKEN_CHAR,
	CYMB_TOKEN_SHORT,
	CYMB_TOKEN_INT,
	CYMB_TOKEN_LONG,
	CYMB_TOKEN_FLOAT,
	CYMB_TOKEN_DOUBLE,
	CYMB_TOKEN__DECIMAL_32,
	CYMB_TOKEN__DECIMAL_64,
	CYMB_TOKEN__DECIMAL_128,
	CYMB_TOKEN__COMPLEX,
	CYMB_TOKEN__IMAGINARY,
	CYMB_TOKEN_SIGNED,
	CYMB_TOKEN_UNSIGNED,
	CYMB_TOKEN_CONST,
	CYMB_TOKEN_CONSTEXPR,
	CYMB_TOKEN_VOLATILE,
	CYMB_TOKEN_STATIC,
	CYMB_TOKEN_EXTERN,
	CYMB_TOKEN_AUTO,
	CYMB_TOKEN_REGISTER,
	CYMB_TOKEN_RESTRICT,
	CYMB_TOKEN_TYPEDEF,
	CYMB_TOKEN_TYPEOF,
	CYMB_TOKEN_TYPEOF_UNQUAL,
	CYMB_TOKEN_STRUCT,
	CYMB_TOKEN_UNION,
	CYMB_TOKEN_ENUM,
	CYMB_TOKEN__ATOMIC,
	CYMB_TOKEN__GENERIC,
	CYMB_TOKEN__BIT_INT,
	CYMB_TOKEN__THREAD_LOCAL,
	CYMB_TOKEN_THREAD_LOCAL,
	CYMB_TOKEN_RETURN,
	CYMB_TOKEN__STATIC_ASSERT,
	CYMB_TOKEN_STATIC_ASSERT,
	CYMB_TOKEN_SIZEOF,
	CYMB_TOKEN__ALIGNAS,
	CYMB_TOKEN_ALIGNAS,
	CYMB_TOKEN__ALIGNOF,
	CYMB_TOKEN_ALIGNOF,
	CYMB_TOKEN_IF,
	CYMB_TOKEN_ELSE,
	CYMB_TOKEN_SWITCH,
	CYMB_TOKEN_CASE,
	CYMB_TOKEN_DEFAULT,
	CYMB_TOKEN_WHILE,
	CYMB_TOKEN_DO,
	CYMB_TOKEN_FOR,
	CYMB_TOKEN_BREAK,
	CYMB_TOKEN_CONTINUE,
	CYMB_TOKEN_GOTO,
	/* End of keywords */
	CYMB_TOKEN_CONSTANT,
	CYMB_TOKEN_OPEN_PARENTHESIS,
	CYMB_TOKEN_CLOSE_PARENTHESIS,
	CYMB_TOKEN_OPEN_BRACE,
	CYMB_TOKEN_CLOSE_BRACE,
	CYMB_TOKEN_OPEN_BRACKET,
	CYMB_TOKEN_CLOSE_BRACKET,
	CYMB_TOKEN_SEMICOLON,
	CYMB_TOKEN_PLUS_EQUAL,
	CYMB_TOKEN_MINUS_EQUAL,
	CYMB_TOKEN_STAR_EQUAL,
	CYMB_TOKEN_SLASH_EQUAL,
	CYMB_TOKEN_PERCENT_EQUAL,
	CYMB_TOKEN_AMPERSAND_EQUAL,
	CYMB_TOKEN_BAR_EQUAL,
	CYMB_TOKEN_CARET_EQUAL,
	CYMB_TOKEN_LEFT_SHIFT_EQUAL,
	CYMB_TOKEN_RIGHT_SHIFT_EQUAL,
	CYMB_TOKEN_LEFT_SHIFT,
	CYMB_TOKEN_RIGHT_SHIFT,
	CYMB_TOKEN_PLUS_PLUS,
	CYMB_TOKEN_MINUS_MINUS,
	CYMB_TOKEN_EQUAL_EQUAL,
	CYMB_TOKEN_NOT_EQUAL,
	CYMB_TOKEN_LESS_EQUAL,
	CYMB_TOKEN_GREATER_EQUAL,
	CYMB_TOKEN_AMPERSAND_AMPERSAND,
	CYMB_TOKEN_BAR_BAR,
	CYMB_TOKEN_PLUS,
	CYMB_TOKEN_MINUS,
	CYMB_TOKEN_STAR,
	CYMB_TOKEN_SLASH,
	CYMB_TOKEN_PERCENT,
	CYMB_TOKEN_AMPERSAND,
	CYMB_TOKEN_BAR,
	CYMB_TOKEN_CARET,
	CYMB_TOKEN_TILDE,
	CYMB_TOKEN_EQUAL,
	CYMB_TOKEN_LESS,
	CYMB_TOKEN_GREATER,
	CYMB_TOKEN_EXCLAMATION,
	CYMB_TOKEN_QUESTION,
	CYMB_TOKEN_COMMA,
	CYMB_TOKEN_DOT,
	CYMB_TOKEN_ARROW,
	CYMB_TOKEN_COLON,
	CYMB_TOKEN_STRING
} CymbTokenType;

/*
 * Check if a token is a keyword.
 *
 * Parameters:
 * - type: A token type.
 *
 * Returns:
 * - true if the token is a keyword.
 * - false otherwise.
 */
bool cymbIsKeyword(CymbTokenType type);

/*
 * A constant type.
 */
typedef enum CymbConstantType
{
	CYMB_CONSTANT_INT,
	CYMB_CONSTANT_LONG,
	CYMB_CONSTANT_LONG_LONG,
	CYMB_CONSTANT_UNSIGNED_INT,
	CYMB_CONSTANT_UNSIGNED_LONG,
	CYMB_CONSTANT_UNSIGNED_LONG_LONG
} CymbConstantType;

/*
 * A constant.
 *
 * Fields:
 * - type: The constant type.
 * - value: The constant value.
 */
typedef struct CymbConstant
{
	CymbConstantType type;
	unsigned long long value;
} CymbConstant;

/*
 * A token.
 *
 * Fields:
 * - type: The token type.
 * - info: The diagnostic info of the token.
 * - constant: The constant info if the token is a constant.
 */
typedef struct CymbToken
{
	CymbTokenType type;

	CymbDiagnosticInfo info;

	CymbConstant constant;
} CymbToken;

/*
 * A list of tokens.
 *
 * Fields:
 * - tokens: An array of tokens.
 * - count: The number of tokens.
 */
typedef struct CymbTokenList
{
	CymbToken* tokens;
	size_t count;
} CymbTokenList;

/*
 * A lex function.
 *
 * Parameters:
 * - reader: A reader.
 * - token: A token in which to store the result.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on match.
 * - CYMB_NO_MATCH if there is no match.
 * - CYMB_INVALID if the token is invalid.
 * - CYMB_OUT_OF_MEMORY if a diagnostic could not be added.
 */
typedef CymbResult (*CymbLexFunction)(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a string literal.
 */
CymbResult cymbParseString(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a character constant.
 */
CymbResult cymbParseCharacter(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a punctuator.
 */
CymbResult cymbParsePunctuator(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse an unsigned integer.
 *
 * Parameters:
 * - reader: A reader.
 * - value: The parsed value.
 * - base: The base.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on match.
 * - CYMB_NO_MATCH if there is no match.
 * - CYMB_INVALID if the number is invalid.
 * - CYMB_OUT_OF_MEMORY if a diagnostic could not be added.
 */
CymbResult cymbParseUnsigned(CymbReader* reader, uintmax_t* value, unsigned char base, CymbDiagnosticList* diagnostics);

/*
 * Parse an integer constant.
 */
CymbResult cymbParseConstant(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse an identifier or keyword.
 */
CymbResult cymbParseIdentifier(CymbReader* reader, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Lex a string into a list of tokens.
 *
 * Parameters:
 * - string: A string.
 * - tokens: A list of tokens.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS if the string is successfully lexed.
 * - CYMB_INVALID if some token is invalid.
 * - CYMB_OUT_OF_MEMORY if memory allocation fails.
 */
CymbResult cymbLex(const char* string, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Free a list of tokens.
 *
 * Parameters:
 * - tokens: A list of tokens.
 */
void cymbFreeTokenList(CymbTokenList* tokens);

#endif
