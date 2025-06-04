#include "cymb/tree.h"

#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

/*
 * Add a node to a tree.
 *
 * Parameters:
 * - builder: A tree builder.
 * - node: The node to add.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if the node could not be added.
 */
static CymbResult cymbAddNode(CymbTreeBuilder* const builder, const CymbNode* const node)
{
	if(builder->tree->count == builder->nodesCapacity)
	{
		if(builder->nodesCapacity * sizeof(builder->tree->nodes[0]) == cymbSizeMax)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}

		builder->nodesCapacity = builder->nodesCapacity * sizeof(builder->tree->nodes[0]) > cymbSizeMax / 2 ? cymbSizeMax : builder->nodesCapacity * 2;
		CymbNode* const newNodes = realloc(builder->tree->nodes, builder->nodesCapacity * sizeof(builder->tree->nodes[0]));
		if(!newNodes)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}
		builder->tree->nodes = newNodes;
	}

	builder->tree->nodes[builder->tree->count] = *node;
	++builder->tree->count;

	return CYMB_SUCCESS;
}

CymbResult cymbSkipParentheses(const CymbConstTokenList* const tokens, const CymbDirection direction, CymbParseResult* const parseResult, size_t* const tokenIndex, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	const CymbTokenType plusType = direction == CYMB_DIRECTION_FORWARD ? CYMB_TOKEN_OPEN_PARENTHESIS : CYMB_TOKEN_CLOSE_PARENTHESIS;
	const CymbTokenType minusType = direction == CYMB_DIRECTION_FORWARD ? CYMB_TOKEN_CLOSE_PARENTHESIS : CYMB_TOKEN_OPEN_PARENTHESIS;

	const size_t startIndex = *tokenIndex;
	const CymbTokenType type = tokens->tokens[startIndex].type;

	if(type == minusType)
	{
		goto error;
	}

	if(type != plusType)
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		goto end;
	}

	size_t parenthesisCount = 1;
	while(direction == CYMB_DIRECTION_FORWARD ? *tokenIndex < tokens->count - 1 : *tokenIndex > 0)
	{
		*tokenIndex += direction;

		const CymbTokenType currentType = tokens->tokens[*tokenIndex].type;

		parenthesisCount += currentType == plusType;
		parenthesisCount -= currentType == minusType;

		if(parenthesisCount == 0)
		{
			*parseResult = CYMB_PARSE_MATCH;
			goto end;
		}
	}

	error:
	*parseResult = CYMB_PARSE_INVALID;
	const CymbDiagnostic diagnostic = {
		.type = CYMB_UNMATCHED_PARENTHESIS,
		.info = tokens->tokens[startIndex].info
	};
	result = cymbDiagnosticAdd(diagnostics, &diagnostic);

	end:
	return result;
}

/*
 * Parse a binary operator.
 *
 * Parameters:
 * - builder: A tree builder.
 * - tokens: The tokens.
 * - operatorTokens: The tokens to match for the operator.
 * - binaryOperators: The corresponding operator types.
 * - count: the number of operators to check.
 * - parseResult: The result.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
static CymbResult cymbParseBinaryOperator(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, const CymbTokenType* const operatorTokens, const CymbBinaryOperator* const binaryOperators, const size_t count, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	size_t tokenIndex = tokens->count - 1;
	while(tokenIndex > 0)
	{
		result = cymbSkipParentheses(tokens, CYMB_DIRECTION_BACKWARD, parseResult, &tokenIndex, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
		{
			return result;
		}
		--tokenIndex;

		size_t matchIndex;
		for(matchIndex = 0; matchIndex < count; ++matchIndex)
		{
			if(tokens->tokens[tokenIndex].type == operatorTokens[matchIndex])
			{
				goto parse;
			}
		}
		continue;

		parse:
		if(tokenIndex == 0 || tokenIndex == tokens->count - 1)
		{
			*parseResult = CYMB_PARSE_NO_MATCH;
			return result;
		}

		result = cymbParseExpression(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens,
			.count = tokenIndex
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
		{
			return result;
		}

		CymbNode* const leftNode = builder->tree->nodes + builder->tree->count - 1;

		result = cymbParseExpression(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens + tokenIndex + 1,
			.count = tokens->count - tokenIndex - 1
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
		{
			return result;
		}

		const CymbNode node = {
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tokens->tokens[tokenIndex].info,
			.binaryOperatorNode = {
				.operator = binaryOperators[matchIndex],
				.leftNode = leftNode,
				.rightNode = builder->tree->nodes + builder->tree->count - 1
			}
		};
		result = cymbAddNode(builder, &node);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		*parseResult = CYMB_PARSE_MATCH;
		return result;
	}

	*parseResult = CYMB_PARSE_NO_MATCH;
	return result;
}

CymbResult cymbParseExpression(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	// Remove potential outer parentheses.
	size_t tokenIndex = 0;
	CymbResult result = cymbSkipParentheses(tokens, CYMB_DIRECTION_FORWARD, parseResult, &tokenIndex, diagnostics);
	if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
	{
		return result;
	}
	if(tokens->tokens[tokenIndex].type == CYMB_TOKEN_CLOSE_PARENTHESIS && tokenIndex == tokens->count - 1)
	{
		if(tokens->count < 3)
		{
			*parseResult = CYMB_PARSE_NO_MATCH;
			return result;
		}

		return cymbParseExpression(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens + 1,
			.count = tokens->count - 2
		}, parseResult, diagnostics);
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_BAR_BAR};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_LOR};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_AMPERSAND_AMPERSAND};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_LAND};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_BAR};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_OR};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_CARET};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_XOR};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_AMPERSAND};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_AND};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_EQUAL_EQUAL, CYMB_TOKEN_NOT_EQUAL};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_EQ, CYMB_BINARY_OPERATOR_NEQ};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_LESS, CYMB_TOKEN_LESS_EQUAL, CYMB_TOKEN_GREATER, CYMB_TOKEN_GREATER_EQUAL};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_LE, CYMB_BINARY_OPERATOR_LEQ, CYMB_BINARY_OPERATOR_GE, CYMB_BINARY_OPERATOR_GEQ};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_LEFT_SHIFT, CYMB_TOKEN_RIGHT_SHIFT};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_LS, CYMB_BINARY_OPERATOR_RS};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_PLUS, CYMB_TOKEN_MINUS};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_SUM, CYMB_BINARY_OPERATOR_DIF};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	{
		constexpr CymbTokenType operatorTokens[] = {CYMB_TOKEN_STAR, CYMB_TOKEN_SLASH, CYMB_TOKEN_PERCENT};
		constexpr CymbBinaryOperator binaryOperators[] = {CYMB_BINARY_OPERATOR_MUL, CYMB_BINARY_OPERATOR_DIV, CYMB_BINARY_OPERATOR_MOD};
		constexpr size_t count = CYMB_LENGTH(operatorTokens);
		static_assert(CYMB_LENGTH(binaryOperators) == count);

		result = cymbParseBinaryOperator(builder, tokens, operatorTokens, binaryOperators, count, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_NO_MATCH)
		{
			return result;
		}
	}

	if(tokens->count == 1 && tokens->tokens[0].type == CYMB_TOKEN_CONSTANT)
	{
		*parseResult = CYMB_PARSE_MATCH;

		const CymbNode node = {
			.type = CYMB_NODE_CONSTANT,
			.info = tokens->tokens[0].info,
			.constantNode = tokens->tokens[0].constant
		};

		return cymbAddNode(builder, &node);
	}

	if(tokens->count == 1 && tokens->tokens[0].type == CYMB_TOKEN_IDENTIFIER)
	{
		*parseResult = CYMB_PARSE_MATCH;

		const CymbNode node = {
			.type = CYMB_NODE_IDENTIFIER,
			.info = tokens->tokens[0].info
		};

		return cymbAddNode(builder, &node);
	}

	*parseResult = CYMB_PARSE_NO_MATCH;
	return result;
}

CymbResult cymbParseType(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	for(size_t pointerIndex = tokens->count - 1; pointerIndex > 0 && tokens->count - pointerIndex <= 3; --pointerIndex)
	{
		if(tokens->tokens[pointerIndex].type == CYMB_TOKEN_STAR)
		{
			CymbPointerNode pointer = {};

			for(size_t qualifierIndex = pointerIndex + 1; qualifierIndex < tokens->count; ++qualifierIndex)
			{
				if(tokens->tokens[qualifierIndex].type == CYMB_TOKEN_CONST)
				{
					if(pointer.isConst)
					{
						*parseResult = CYMB_PARSE_INVALID;

						const CymbDiagnostic diagnostic = {
							.type = CYMB_DOUBLE_CONST,
							.info = tokens->tokens[qualifierIndex].info
						};

						return cymbDiagnosticAdd(diagnostics, &diagnostic);
					}

					pointer.isConst = true;
				}

				else if(tokens->tokens[qualifierIndex].type == CYMB_TOKEN_RESTRICT)
				{
					if(pointer.isRestrict)
					{
						*parseResult = CYMB_PARSE_INVALID;

						const CymbDiagnostic diagnostic = {
							.type = CYMB_DOUBLE_RESTRICT,
							.info = tokens->tokens[qualifierIndex].info
						};

						return cymbDiagnosticAdd(diagnostics, &diagnostic);
					}

					pointer.isRestrict = true;
				}

				else
				{
					*parseResult = CYMB_PARSE_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_UNEXPECTED_TOKEN,
						.info = tokens->tokens[qualifierIndex].info
					};

					return cymbDiagnosticAdd(diagnostics, &diagnostic);
				}
			}

			result = cymbParseType(builder, &(CymbConstTokenList){
				.tokens = tokens->tokens,
				.count = pointerIndex
			}, parseResult, diagnostics);
			if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
			{
				return result;
			}

			pointer.pointedNode = builder->tree->nodes + builder->tree->count - 1;

			*parseResult = CYMB_PARSE_MATCH;

			const CymbNode node = {
				.type = CYMB_NODE_POINTER,
				.pointerNode = pointer,
				.info = tokens->tokens[pointerIndex].info
			};
			return cymbAddNode(builder, &node);
		}
	}

	CymbTypeNode type = {};
	CymbDiagnosticInfo info;

	if(tokens->tokens[0].type == CYMB_TOKEN_STATIC)
	{
		type.isStatic = true;

		++tokens->tokens;
		--tokens->count;
		if(tokens->count == 0)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}
	}

	if(tokens->tokens[0].type == CYMB_TOKEN_CONST)
	{
		type.isConst = true;

		++tokens->tokens;
		--tokens->count;
		if(tokens->count == 0)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}
	}

	if(tokens->tokens[tokens->count - 1].type == CYMB_TOKEN_CONST)
	{
		if(type.isConst)
		{
			*parseResult = CYMB_PARSE_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_DOUBLE_CONST,
				.info = tokens->tokens[tokens->count - 1].info
			};

			return cymbDiagnosticAdd(diagnostics, &diagnostic);
		}

		type.isConst = true;

		--tokens->count;
		if(tokens->count == 0)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}
	}

	if(tokens->count == 3)
	{
		if(tokens->tokens[1].type != CYMB_TOKEN_LONG)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		if
		(
			(tokens->tokens[0].type == CYMB_TOKEN_LONG && tokens->tokens[2].type == CYMB_TOKEN_UNSIGNED) ||
			(tokens->tokens[2].type == CYMB_TOKEN_LONG && tokens->tokens[0].type == CYMB_TOKEN_UNSIGNED)
		)
		{
			type.type = CYMB_TYPE_UNSIGNED_LONG_LONG;
			info = tokens->tokens[1 + (tokens->tokens[2].type == CYMB_TOKEN_LONG)].info;
			goto append;
		}

		if
		(
			(tokens->tokens[0].type == CYMB_TOKEN_LONG && tokens->tokens[2].type == CYMB_TOKEN_SIGNED) ||
			(tokens->tokens[2].type == CYMB_TOKEN_LONG && tokens->tokens[0].type == CYMB_TOKEN_SIGNED)
		)
		{
			type.type = CYMB_TYPE_LONG_LONG;
			info = tokens->tokens[1 + (tokens->tokens[2].type == CYMB_TOKEN_LONG)].info;
			goto append;
		}

		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	if(tokens->count == 2)
	{
		if(tokens->tokens[0].type == CYMB_TOKEN_LONG && tokens->tokens[1].type == CYMB_TOKEN_LONG)
		{
			type.type = CYMB_TYPE_LONG_LONG;
			info = tokens->tokens[1].info;
			goto append;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_STRUCT)
		{
			if(tokens->tokens[1].type != CYMB_TOKEN_IDENTIFIER)
			{
				*parseResult = CYMB_PARSE_INVALID;
				return result;
			}

			type.type = CYMB_TYPE_STRUCT;
			info = tokens->tokens[1].info;

			goto append;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_ENUM)
		{
			if(tokens->tokens[1].type != CYMB_TOKEN_IDENTIFIER)
			{
				*parseResult = CYMB_PARSE_INVALID;
				return result;
			}

			type.type = CYMB_TYPE_ENUM;
			info = tokens->tokens[1].info;

			goto append;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_UNION)
		{
			if(tokens->tokens[1].type != CYMB_TOKEN_IDENTIFIER)
			{
				*parseResult = CYMB_PARSE_INVALID;
				return result;
			}

			type.type = CYMB_TYPE_UNION;
			info = tokens->tokens[1].info;

			goto append;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_UNSIGNED || tokens->tokens[1].type == CYMB_TOKEN_UNSIGNED)
		{
			const size_t typeIndex = tokens->tokens[0].type == CYMB_TOKEN_UNSIGNED;
			switch(tokens->tokens[typeIndex].type)
			{
				case CYMB_TOKEN_CHAR:
					type.type = CYMB_TYPE_UNSIGNED_CHAR;
					break;

				case CYMB_TOKEN_SHORT:
					type.type = CYMB_TYPE_UNSIGNED_SHORT;
					break;

				case CYMB_TOKEN_INT:
					type.type = CYMB_TYPE_UNSIGNED_INT;
					break;

				case CYMB_TOKEN_LONG:
					type.type = CYMB_TYPE_UNSIGNED_LONG;
					break;

				default:
					*parseResult = CYMB_PARSE_INVALID;
					return result;
			}

			info = tokens->tokens[typeIndex].info;

			goto append;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_SIGNED || tokens->tokens[1].type == CYMB_TOKEN_SIGNED)
		{
			const size_t typeIndex = tokens->tokens[0].type == CYMB_TOKEN_SIGNED;
			switch(tokens->tokens[typeIndex].type)
			{
				case CYMB_TOKEN_CHAR:
					type.type = CYMB_TYPE_SIGNED_CHAR;
					break;

				case CYMB_TOKEN_SHORT:
					type.type = CYMB_TYPE_SHORT;
					break;

				case CYMB_TOKEN_INT:
					type.type = CYMB_TYPE_INT;
					break;

				case CYMB_TOKEN_LONG:
					type.type = CYMB_TYPE_LONG;
					break;

				default:
					*parseResult = CYMB_PARSE_INVALID;
					return result;
			}

			info = tokens->tokens[typeIndex].info;

			goto append;
		}

		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	if(tokens->count != 1)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	switch(tokens->tokens[0].type)
	{
		case CYMB_TOKEN_CHAR:
			type.type = CYMB_TYPE_CHAR;
			break;

		case CYMB_TOKEN_VOID:
			type.type = CYMB_TYPE_VOID;
			break;

		case CYMB_TOKEN_SHORT:
			type.type = CYMB_TYPE_SHORT;
			break;

		case CYMB_TOKEN_INT:
			type.type = CYMB_TYPE_INT;
			break;

		case CYMB_TOKEN_LONG:
			type.type = CYMB_TYPE_LONG;
			break;

		case CYMB_TOKEN_FLOAT:
			type.type = CYMB_TYPE_FLOAT;
			break;

		case CYMB_TOKEN_DOUBLE:
			type.type = CYMB_TYPE_DOUBLE;
			break;

		case CYMB_TOKEN__BOOL:
		case CYMB_TOKEN_BOOL:
			type.type = CYMB_TYPE_BOOL;
			break;

		case CYMB_TOKEN_IDENTIFIER:
			type.type = CYMB_TYPE_OTHER;
			break;

		default:
			*parseResult = CYMB_PARSE_INVALID;
			return result;
	}

	info = tokens->tokens[0].info;

	append:
	*parseResult = CYMB_PARSE_MATCH;

	const CymbNode node = {
		.type = CYMB_NODE_TYPE,
		.typeNode = type,
		.info = info
	};
	return cymbAddNode(builder, &node);
}

CymbResult cymbParseStatement(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	const CymbToken* endToken = tokens->tokens;
	while(endToken < tokens->tokens + tokens->count)
	{
		if(endToken->type == CYMB_TOKEN_SEMICOLON)
		{
			break;
		}

		++endToken;
	}
	if(endToken == tokens->tokens + tokens->count)
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return result;
	}

	if(tokens->tokens[0].type == CYMB_TOKEN_RETURN)
	{
		const bool empty = endToken == tokens->tokens + 1;

		if(!empty)
		{
			result = cymbParseExpression(builder, &(CymbConstTokenList){
				.tokens = tokens->tokens + 1,
				.count = endToken - tokens->tokens - 1
			}, parseResult, diagnostics);
			if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
			{
				return result;
			}
		}

		const CymbNode node = {
			.type = CYMB_NODE_RETURN,
			.returnNode = empty ? nullptr : builder->tree->nodes + builder->tree->count - 1,
			.info = tokens->tokens[0].info
		};
		result = cymbAddNode(builder, &node);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		goto append;
	}

	if(tokens->count >= 3)
	{
		const CymbToken* equalToken = tokens->tokens;
		while(equalToken < endToken)
		{
			if(equalToken->type == CYMB_TOKEN_EQUAL)
			{
				break;
			}

			++equalToken;
		}
		const bool hasInitializer = equalToken < endToken;

		if(hasInitializer && equalToken == endToken - 1)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		if(equalToken < tokens->tokens + 2)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		const CymbToken* const identifierToken = equalToken - 1;
		if(identifierToken->type != CYMB_TOKEN_IDENTIFIER)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		const CymbToken* typeToken = identifierToken;
		do
		{
			--typeToken;

			if
			(
				typeToken->type != CYMB_TOKEN_IDENTIFIER &&
				!cymbIsKeyword(typeToken->type) &&
				typeToken->type != CYMB_TOKEN_STAR
			)
			{
				*parseResult = CYMB_PARSE_INVALID;
				return result;
			}
		} while(typeToken > tokens->tokens);

		result = cymbParseType(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens,
			.count = equalToken - tokens->tokens - 1
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
		{
			return result;
		}
		CymbNode* const typeNode = builder->tree->nodes + builder->tree->count - 1;

		const CymbNode identifierNode = {
			.type = CYMB_NODE_IDENTIFIER,
			.info = identifierToken->info
		};
		result = cymbAddNode(builder, &identifierNode);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
		CymbNode* const identifierNodePointer = builder->tree->nodes + builder->tree->count - 1;

		if(hasInitializer)
		{
			result = cymbParseExpression(builder, &(CymbConstTokenList){
				.tokens = equalToken + 1,
				.count = endToken - equalToken - 1
			}, parseResult, diagnostics);
			if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
			{
				return result;
			}
		}

		*parseResult = CYMB_PARSE_MATCH;

		const CymbNode node = {
			.type = CYMB_NODE_DECLARATION,
			.declarationNode = {
				.identifier = identifierNodePointer,
				.type = typeNode,
				.initializer = hasInitializer ? builder->tree->nodes + builder->tree->count - 1 : nullptr
			},
			.info = identifierToken->info
		};
		result = cymbAddNode(builder, &node);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		goto append;
	}

	*parseResult = CYMB_PARSE_NO_MATCH;
	return result;

	append:
	*parseResult = CYMB_PARSE_MATCH;

	tokens->count -= endToken + 1 - tokens->tokens;
	tokens->tokens = endToken + 1;

	return result;
}

/*
 * Store a child at the end of a tree's children array.
 *
 * Parameters:
 * - builder: A tree builder.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if the child could not be stored.
 */
static CymbResult cymbStoreChild(CymbTreeBuilder* const builder)
{
	if(builder->lastStored == builder->childCount)
	{
		if(builder->childrenCapacity * sizeof(builder->tree->children[0]) == cymbSizeMax)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}

		const size_t storedCount = builder->childrenCapacity - builder->lastStored;

		builder->childrenCapacity = builder->childrenCapacity * sizeof(builder->tree->children[0]) > cymbSizeMax / 2 ? cymbSizeMax : builder->childrenCapacity * 2;

		CymbNode** const newChildren = realloc(builder->tree->children, builder->childrenCapacity * sizeof(builder->tree->children[0]));
		if(!newChildren)
		{
			return CYMB_ERROR_OUT_OF_MEMORY;
		}
		builder->tree->children = newChildren;

		const size_t newLastStored = builder->childrenCapacity - storedCount;
		if(storedCount > 0)
		{
			memmove(
				builder->tree->children + newLastStored,
				builder->tree->children + builder->lastStored,
				storedCount * sizeof(builder->tree->children[0])
			);
		}
		builder->lastStored = newLastStored;
	}

	--builder->lastStored;
	builder->tree->children[builder->lastStored] = builder->tree->nodes + builder->tree->count - 1;

	return CYMB_SUCCESS;
}

/*
 * Move a number of children from storage to the actual array.
 *
 * Parameters:
 * - builder: A tree builder.
 * - childCount: The number of children to move.
 */
static void cymbCommitChildren(CymbTreeBuilder* const builder, const size_t childCount)
{
	for(size_t storedIndex = 0; storedIndex < childCount; ++storedIndex)
	{
		builder->tree->children[builder->childCount + storedIndex] = builder->tree->children[builder->lastStored + childCount - 1 - storedIndex];
	}

	builder->childCount += childCount;
	builder->lastStored += childCount;
}

CymbResult cymbParseFunction(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	// Find function name.
	const CymbToken* name = tokens->tokens;
	while
	(
		name < tokens->tokens + tokens->count &&
		(cymbIsKeyword(name->type) || name->type == CYMB_TOKEN_STAR || name->type == CYMB_TOKEN_IDENTIFIER)
	)
	{
		++name;
	}
	if(name == tokens->tokens || name == tokens->tokens + tokens->count || name->type != CYMB_TOKEN_OPEN_PARENTHESIS)
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return result;
	}

	--name;
	if(name->type != CYMB_TOKEN_IDENTIFIER)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	// Parse return type.
	const size_t typeCount = name - tokens->tokens;

	result = cymbParseType(builder, &(CymbConstTokenList){
		.tokens = tokens->tokens,
		.count = typeCount
	}, parseResult, diagnostics);

	if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	CymbNode* const returnTypeNode = builder->tree->nodes + builder->tree->count - 1;

	// Parse parameters.
	const size_t parametersOffset = typeCount + 2;
	tokens->count -= parametersOffset;
	tokens->tokens += parametersOffset;
	if(tokens->count == 0)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	size_t parameterCount = 0;
	if(tokens->count >= 2 && tokens->tokens[0].type == CYMB_TOKEN_VOID && tokens->tokens[1].type == CYMB_TOKEN_CLOSE_PARENTHESIS)
	{
		tokens->count -= 1;
		tokens->tokens += 1;
	}
	while(tokens->count > 0 && tokens->tokens->type != CYMB_TOKEN_CLOSE_PARENTHESIS)
	{
		const CymbToken* parameterName = tokens->tokens;
		while
		(
			parameterName < tokens->tokens + tokens->count &&
			(cymbIsKeyword(parameterName->type) || parameterName->type == CYMB_TOKEN_STAR || parameterName->type == CYMB_TOKEN_IDENTIFIER)
		)
		{
			++parameterName;
		}
		if(parameterName == tokens->tokens + tokens->count || parameterName == tokens->tokens || (parameterName->type != CYMB_TOKEN_COMMA && parameterName->type != CYMB_TOKEN_CLOSE_PARENTHESIS))
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		--parameterName;
		if(parameterName->type != CYMB_TOKEN_IDENTIFIER)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		const size_t parameterTypeCount = parameterName - tokens->tokens;
		result = cymbParseType(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens,
			.count = parameterTypeCount
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		const CymbNode parameterNameNode = {
			.type = CYMB_NODE_IDENTIFIER,
			.info = parameterName->info
		};
		result = cymbAddNode(builder, &parameterNameNode);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		const size_t parameterOffset = parameterTypeCount + 1 + ((parameterName + 1)->type == CYMB_TOKEN_COMMA);
		tokens->count -= parameterOffset;
		tokens->tokens += parameterOffset;

		++parameterCount;
	}

	if(tokens->count == 0)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	const CymbNode type = {
		.type = CYMB_NODE_FUNCTION_TYPE,
		.functionTypeNode = {
			.returnType = returnTypeNode,
			.parameterTypesStart = parameterCount == 0 ? nullptr : builder->tree->children + builder->childCount,
			.parameterTypesCount = parameterCount
		},
		.info = returnTypeNode->info
	};
	result = cymbAddNode(builder, &type);
	if(result != CYMB_SUCCESS)
	{
		return result;
	}
	CymbNode* const typeNodePointer = builder->tree->nodes + builder->tree->count - 1;

	for(size_t parameterIndex = 0; parameterIndex < parameterCount; ++parameterIndex)
	{
		CymbNode* const parameterType = builder->tree->children[builder->lastStored + 2 * parameterCount - parameterIndex - 1];

		memmove(builder->tree->children + builder->lastStored + parameterIndex + 1, builder->tree->children + builder->lastStored + parameterIndex, (2 * parameterCount - 1 - 2 * parameterIndex) * sizeof(builder->tree->children[0]));

		builder->tree->children[builder->childCount + parameterIndex] = parameterType;
	}
	builder->childCount += parameterCount;
	builder->lastStored += parameterCount;

	CymbNode** const parametersStart = builder->tree->children + builder->childCount;
	cymbCommitChildren(builder, parameterCount);

	const CymbNode nameNode = {
		.type = CYMB_NODE_IDENTIFIER,
		.info = name->info
	};
	result = cymbAddNode(builder, &nameNode);
	if(result != CYMB_SUCCESS)
	{
		return result;
	}
	CymbNode* const nameNodePointer = builder->tree->nodes + builder->tree->count - 1;

	++tokens->tokens;
	--tokens->count;

	// Parse statements.
	if(tokens->count == 0 || tokens->tokens->type != CYMB_TOKEN_OPEN_BRACE)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	++tokens->tokens;
	--tokens->count;

	const CymbToken* end = tokens->tokens;
	while(end < tokens->tokens + tokens->count && end->type != CYMB_TOKEN_CLOSE_BRACE)
	{
		++end;
	}
	if(end == tokens->tokens + tokens->count)
	{
		*parseResult = CYMB_PARSE_INVALID;
		return result;
	}

	CymbNode** const statementsStart = builder->tree->children + builder->childCount;

	size_t statementsTokenCount = end - tokens->tokens;
	CymbConstTokenList statementsTokens = {
		.tokens = tokens->tokens,
		.count = end - tokens->tokens
	};
	size_t statementCount = 0;
	while(statementsTokens.count > 0)
	{
		result = cymbParseStatement(builder, &statementsTokens, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			return result;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		++statementCount;
	}

	if(statementCount > 0)
	{
		cymbCommitChildren(builder, statementCount);
	}

	tokens->count -= statementsTokenCount + 1;
	tokens->tokens = end + 1;

	const CymbNode node = {
		.type = CYMB_NODE_FUNCTION,
		.functionNode = {
			.name = nameNodePointer,
			.type = typeNodePointer,
			.parametersStart = parameterCount == 0 ? nullptr : parametersStart,
			.parametersCount = parameterCount,
			.statementsStart = statementCount == 0 ? nullptr : statementsStart,
			.statementsCount = statementCount
		},
		.info = nameNode.info
	};
	result = cymbAddNode(builder, &node);
	if(result != CYMB_SUCCESS)
	{
		return result;
	}

	*parseResult = CYMB_PARSE_MATCH;
	return result;
}

CymbResult cymbParseProgram(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	size_t childCount = 0;
	while(tokens->count > 0)
	{
		result = cymbParseFunction(builder, tokens, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
		{
			return result;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		++childCount;
	}

	if(childCount > 0)
	{
		cymbCommitChildren(builder, childCount);
	}

	const CymbNode node = {
		.type = CYMB_NODE_PROGRAM,
		.programNode = {
			.childrenStart = builder->tree->children + builder->childCount - childCount,
			.childrenCount = childCount
		}
	};
	result = cymbAddNode(builder, &node);
	if(result != CYMB_SUCCESS)
	{
		return result;
	}

	*parseResult = tokens->count == 0 ? CYMB_PARSE_MATCH : CYMB_PARSE_INVALID;
	return result;
}

CymbResult cymbParse(const CymbConstTokenList* const tokens, CymbTree* const tree, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	tree->nodes = nullptr;
	tree->children = nullptr;
	tree->count = 0;

	CymbTreeBuilder builder = {
		.tree = tree,
		.nodesCapacity = 1024,
		.childrenCapacity = 256,
		.lastStored = builder.childrenCapacity
	};

	tree->nodes = malloc(builder.nodesCapacity * sizeof(tree->nodes[0]));
	if(!tree->nodes)
	{
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto error;
	}

	tree->children = malloc(builder.childrenCapacity * sizeof(tree->children[0]));
	if(!tree->children)
	{
		result = CYMB_ERROR_OUT_OF_MEMORY;
		goto error;
	}

	CymbParseResult parseResult;
	result = cymbParseProgram(&builder, &(CymbConstTokenList){
		.tokens = tokens->tokens,
		.count = tokens->count
	}, &parseResult, diagnostics);
	if(result != CYMB_SUCCESS || parseResult == CYMB_PARSE_INVALID)
	{
		result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
		goto error;
	}

	if(tree->count == 0)
	{
		free(tree->nodes);
		tree->nodes = nullptr;
	}

	if(builder.childCount == 0)
	{
		free(tree->children);
		tree->nodes = nullptr;
	}

	goto end;

	error:
	free(tree->children);
	free(tree->nodes);

	end:
	return result;
}

void cymbFreeTree(CymbTree* const tree)
{
	free(tree->nodes);
	free(tree->children);
}
