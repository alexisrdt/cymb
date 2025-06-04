#ifndef CYMB_LEX_H
#define CYMB_LEX_H

#include <stddef.h>

#include "cymb/diagnostic.h"
#include "cymb/memory.h"
#include "cymb/reader.h"
#include "cymb/result.h"

#define CYMB_TOKEN(F) \
	F(IDENTIFIER) \
	/* Keywords */ \
	F(VOID) \
	F(_BOOL) \
	F(BOOL) \
	F(FALSE) \
	F(TRUE) \
	F(CHAR) \
	F(SHORT) \
	F(INT) \
	F(LONG) \
	F(FLOAT) \
	F(DOUBLE) \
	F(_DECIMAL_32) \
	F(_DECIMAL_64) \
	F(_DECIMAL_128) \
	F(_COMPLEX) \
	F(_IMAGINARY) \
	F(SIGNED) \
	F(UNSIGNED) \
	F(CONST) \
	F(CONSTEXPR) \
	F(VOLATILE) \
	F(STATIC) \
	F(EXTERN) \
	F(AUTO) \
	F(REGISTER) \
	F(RESTRICT) \
	F(TYPEDEF) \
	F(TYPEOF) \
	F(TYPEOF_UNQUAL) \
	F(STRUCT) \
	F(UNION) \
	F(ENUM) \
	F(_ATOMIC) \
	F(_GENERIC) \
	F(_BIT_INT) \
	F(_THREAD_LOCAL) \
	F(THREAD_LOCAL) \
	F(RETURN) \
	F(_STATIC_ASSERT) \
	F(STATIC_ASSERT) \
	F(SIZEOF) \
	F(_ALIGNAS) \
	F(ALIGNAS) \
	F(_ALIGNOF) \
	F(ALIGNOF) \
	F(IF) \
	F(ELSE) \
	F(SWITCH) \
	F(CASE) \
	F(DEFAULT) \
	F(WHILE) \
	F(DO) \
	F(FOR) \
	F(BREAK) \
	F(CONTINUE) \
	F(GOTO) \
	/* End of keywords */ \
	F(CONSTANT) \
	F(OPEN_PARENTHESIS) \
	F(CLOSE_PARENTHESIS) \
	F(OPEN_BRACE) \
	F(CLOSE_BRACE) \
	F(OPEN_BRACKET) \
	F(CLOSE_BRACKET) \
	F(SEMICOLON) \
	F(PLUS_EQUAL) \
	F(MINUS_EQUAL) \
	F(STAR_EQUAL) \
	F(SLASH_EQUAL) \
	F(PERCENT_EQUAL) \
	F(AMPERSAND_EQUAL) \
	F(BAR_EQUAL) \
	F(CARET_EQUAL) \
	F(LEFT_SHIFT_EQUAL) \
	F(RIGHT_SHIFT_EQUAL) \
	F(LEFT_SHIFT) \
	F(RIGHT_SHIFT) \
	F(PLUS_PLUS) \
	F(MINUS_MINUS) \
	F(EQUAL_EQUAL) \
	F(NOT_EQUAL) \
	F(LESS_EQUAL) \
	F(GREATER_EQUAL) \
	F(AMPERSAND_AMPERSAND) \
	F(BAR_BAR) \
	F(PLUS) \
	F(MINUS) \
	F(STAR) \
	F(SLASH) \
	F(PERCENT) \
	F(AMPERSAND) \
	F(BAR) \
	F(CARET) \
	F(TILDE) \
	F(EQUAL) \
	F(LESS) \
	F(GREATER) \
	F(EXCLAMATION) \
	F(QUESTION) \
	F(COMMA) \
	F(DOT) \
	F(ARROW) \
	F(COLON) \
	F(STRING)

#define CYMB_TOKEN_ENUM(name) \
	CYMB_TOKEN_##name,

/*
 * A token type.
 */
typedef enum CymbTokenType
{
	CYMB_TOKEN(CYMB_TOKEN_ENUM)
} CymbTokenType;

/*
 * Get string representation of a token type.
 *
 * Parameters:
 * - type: A token type.
 *
 * Returns:
 * - A string representation of the token type.
 */
const char* cymbTokenTypeString(CymbTokenType type);

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
 * A list of constant tokens.
 *
 * Fields:
 * - tokens: An array of tokens.
 * - count: The number of tokens.
 */
typedef struct CymbConstTokenList
{
	const CymbToken* tokens;
	size_t count;
} CymbConstTokenList;

/*
 * A parse result.
 */
typedef enum CymbParseResult
{
	CYMB_PARSE_MATCH,
	CYMB_PARSE_NO_MATCH,
	CYMB_PARSE_INVALID,
} CymbParseResult;

/*
 * A lex function.
 *
 * Parameters:
 * - reader: A reader.
 * - parseResult: The result.
 * - token: A token in which to store the result.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if a diagnostic could not be added.
 */
typedef CymbResult (*CymbLexFunction)(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a string literal.
 */
CymbResult cymbParseString(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a character constant.
 */
CymbResult cymbParseCharacter(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse a token that does not fit in another category (operators, punctuation).
 */
CymbResult cymbParseToken(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse an integer constant.
 */
CymbResult cymbParseConstant(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

/*
 * Parse an identifier or keyword.
 */
CymbResult cymbParseIdentifier(CymbReader* reader, CymbParseResult* parseResult, CymbToken* token, CymbDiagnosticList* diagnostics);

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
 * - CYMB_ERROR_OUT_OF_MEMORY if memory allocation fails.
 * - CYMB_ERROR_INVALID_ARGUMENT if some token is invalid.
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
