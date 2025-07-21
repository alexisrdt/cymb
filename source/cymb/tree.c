#include "cymb/tree.h"

#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

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

typedef enum CymbAssociativity
{
	CYMB_LEFT_TO_RIGHT,
	CYMB_RIGHT_TO_LEFT
} CymbAssociativity;

typedef struct CymbBinaryOperatorMapping
{
	CymbTokenType token;
	CymbBinaryOperator operator;

	unsigned char precedence;
	CymbAssociativity associativity;
} CymbBinaryOperatorMapping;

constexpr CymbBinaryOperatorMapping binaryOperators[] = {
	{CYMB_TOKEN_STAR, CYMB_BINARY_OPERATOR_MULTIPLICATION, 11, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_SLASH, CYMB_BINARY_OPERATOR_DIVISION, 11, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_PERCENT, CYMB_BINARY_OPERATOR_REMAINDER, 11, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_PLUS, CYMB_BINARY_OPERATOR_ADDITION, 10, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_MINUS, CYMB_BINARY_OPERATOR_SUBTRACTION, 10, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_LEFT_SHIFT, CYMB_BINARY_OPERATOR_LESSFT_SHIFT, 9, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_RIGHT_SHIFT, CYMB_BINARY_OPERATOR_RIGHT_SHIFT, 9, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_LESS, CYMB_BINARY_OPERATOR_LESS, 8, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_LESS_EQUAL, CYMB_BINARY_OPERATOR_LESS_EQUAL, 8, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_GREATER, CYMB_BINARY_OPERATOR_GREATER, 8, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_GREATER_EQUAL, CYMB_BINARY_OPERATOR_GREATER_EQUAL, 8, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_EQUAL_EQUAL, CYMB_BINARY_OPERATOR_EQUAL, 7, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_NOT_EQUAL, CYMB_BINARY_OPERATOR_NOT_EQUAL, 7, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_AMPERSAND, CYMB_BINARY_OPERATOR_BITWISE_AND, 6, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_CARET, CYMB_BINARY_OPERATOR_BITWISE_EXCLUSIVE_OR, 5, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_BAR, CYMB_BINARY_OPERATOR_BITWISE_OR, 4, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_AMPERSAND_AMPERSAND, CYMB_BINARY_OPERATOR_LOGICAL_AND, 3, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_BAR_BAR, CYMB_BINARY_OPERATOR_LOGICAL_OR, 2, CYMB_LEFT_TO_RIGHT},
	{CYMB_TOKEN_EQUAL, CYMB_BINARY_OPERATOR_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_PLUS_EQUAL, CYMB_BINARY_OPERATOR_ADDITION_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_MINUS_EQUAL, CYMB_BINARY_OPERATOR_SUBTRACTION_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_STAR_EQUAL, CYMB_BINARY_OPERATOR_MULTIPLICATION_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_SLASH_EQUAL, CYMB_BINARY_OPERATOR_DIVISION_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_PERCENT_EQUAL, CYMB_BINARY_OPERATOR_REMAINDER_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_LEFT_SHIFT_EQUAL, CYMB_BINARY_OPERATOR_LEFT_SHIFT_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_RIGHT_SHIFT_EQUAL, CYMB_BINARY_OPERATOR_RIGHT_SHIFT_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_AMPERSAND_EQUAL, CYMB_BINARY_OPERATOR_BITWISE_AND_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_CARET_EQUAL, CYMB_BINARY_OPERATOR_BITWISE_EXCLUSIVE_OR_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT},
	{CYMB_TOKEN_BAR_EQUAL, CYMB_BINARY_OPERATOR_BITWISE_OR_ASSIGNMENT, 1, CYMB_RIGHT_TO_LEFT}
};
constexpr size_t binaryOperatorCount = CYMB_LENGTH(binaryOperators);
constexpr size_t binaryOperatorSize = sizeof(binaryOperators[0]);

static int cymbCompareBinaryOperator(const void* const tokenVoid, const void* const operatorVoid)
{
	const CymbTokenType token = *(const CymbTokenType*)tokenVoid;
	const CymbBinaryOperatorMapping* operator = operatorVoid;

	return token != operator->token;
}

typedef struct CymbUnaryOperatorMapping
{
	CymbTokenType token;
	CymbUnaryOperator operator;
} CymbUnaryOperatorMapping;

constexpr CymbUnaryOperatorMapping unaryOperators[] = {
	{CYMB_TOKEN_PLUS, CYMB_UNARY_OPERATOR_POSITIVE},
	{CYMB_TOKEN_MINUS, CYMB_UNARY_OPERATOR_NEGATIVE},
	{CYMB_TOKEN_PLUS_PLUS, CYMB_UNARY_OPERATOR_INCREMENT},
	{CYMB_TOKEN_MINUS_MINUS, CYMB_UNARY_OPERATOR_DECREMENT},
	{CYMB_TOKEN_TILDE, CYMB_UNARY_OPERATOR_BITWISE_NOT},
	{CYMB_TOKEN_EXCLAMATION, CYMB_UNARY_OPERATOR_LOGICAL_NOT},
	{CYMB_TOKEN_AMPERSAND, CYMB_UNARY_OPERATOR_ADDRESS},
	{CYMB_TOKEN_STAR, CYMB_UNARY_OPERATOR_INDIRECTION}
};
constexpr size_t unaryOperatorCount = CYMB_LENGTH(unaryOperators);
constexpr size_t unaryOperatorSize = sizeof(unaryOperators[0]);

static int cymbCompareUnaryOperator(const void* const tokenVoid, const void* const operatorVoid)
{
	const CymbTokenType token = *(const CymbTokenType*)tokenVoid;
	const CymbUnaryOperatorMapping* operator = operatorVoid;

	return token != operator->token;
}

static CymbResult cymbParseSubexpression(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics, const unsigned char minimumPrecedence)
{
	CymbResult result = CYMB_SUCCESS;
	*parseResult = CYMB_PARSE_MATCH;

	if(tokens->tokens[0].type == CYMB_TOKEN_OPEN_PARENTHESIS)
	{
		const CymbToken* const parenthesisToken = tokens->tokens;

		++tokens->tokens;
		--tokens->count;

		result = cymbParseSubexpression(builder, tokens, parseResult, diagnostics, 0);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			return result;
		}

		if(tokens->count == 0)
		{
			*parseResult = CYMB_PARSE_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNMATCHED_PARENTHESIS,
				.info = parenthesisToken->info
			};
			return cymbDiagnosticAdd(diagnostics, &diagnostic);
		}

		++tokens->tokens;
		--tokens->count;
	}
	else
	{
		const CymbUnaryOperatorMapping* const operator = cymbFind(&tokens->tokens[0].type, unaryOperators, unaryOperatorCount, unaryOperatorSize, cymbCompareUnaryOperator);
		if(operator)
		{
			CymbNode node = {
				.type = CYMB_NODE_UNARY_OPERATOR,
				.info = tokens->tokens[0].info,
				.unaryOperatorNode = {
					.operator = operator->operator
				}
			};

			++tokens->tokens;
			--tokens->count;

			result = cymbParseSubexpression(builder, tokens, parseResult, diagnostics, binaryOperators[0].precedence + 1);
			if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
			{
				return result;
			}

			node.unaryOperatorNode.node = builder->tree->nodes + builder->tree->count - 1;

			result = cymbAddNode(builder, &node);
			if(result != CYMB_SUCCESS)
			{
				return result;
			}
		}
		else
		{
			if(tokens->tokens[0].type != CYMB_TOKEN_CONSTANT && tokens->tokens[0].type != CYMB_TOKEN_IDENTIFIER)
			{
				*parseResult = CYMB_PARSE_INVALID;
				return result;
			}

			CymbNode leftNode = {
				.type =  tokens->tokens[0].type == CYMB_TOKEN_CONSTANT ? CYMB_NODE_CONSTANT : CYMB_NODE_IDENTIFIER,
				.info = tokens->tokens[0].info
			};
			if(tokens->tokens[0].type == CYMB_TOKEN_CONSTANT)
			{
				leftNode.constantNode = tokens->tokens[0].constant;
			}
			result = cymbAddNode(builder, &leftNode);
			if(result != CYMB_SUCCESS)
			{
				return result;
			}

			++tokens->tokens;
			--tokens->count;
		}
	}

	while(tokens->count > 0)
	{
		if(tokens->tokens[0].type == CYMB_TOKEN_CLOSE_PARENTHESIS)
		{
			return result;
		}

		const CymbBinaryOperatorMapping* const operator = cymbFind(&tokens->tokens[0].type, binaryOperators, binaryOperatorCount, binaryOperatorSize, cymbCompareBinaryOperator);
		if(!operator)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		if(operator->precedence < minimumPrecedence || (operator->associativity == CYMB_LEFT_TO_RIGHT && operator->precedence == minimumPrecedence))
		{
			return result;
		}

		CymbNode operatorNode = {
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tokens->tokens[0].info,
			.binaryOperatorNode = {
				.operator = operator->operator,
				.leftNode = builder->tree->nodes + builder->tree->count - 1
			}
		};

		++tokens->tokens;
		--tokens->count;

		result = cymbParseSubexpression(builder, tokens, parseResult, diagnostics, operator->precedence);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			return result;
		}

		operatorNode.binaryOperatorNode.rightNode = builder->tree->nodes + builder->tree->count - 1;

		result = cymbAddNode(builder, &operatorNode);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
	}

	return result;
}

CymbResult cymbParseExpression(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	return cymbParseSubexpression(builder, tokens, parseResult, diagnostics, 0);
}

CymbResult cymbParseType(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;
	*parseResult = CYMB_PARSE_MATCH;

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
							.type = CYMB_MULTIPLE_CONST,
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
							.type = CYMB_MULTIPLE_RESTRICT,
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

	while(tokens->tokens[0].type == CYMB_TOKEN_STATIC || tokens->tokens[0].type == CYMB_TOKEN_CONST)
	{
		if(tokens->tokens[0].type == CYMB_TOKEN_STATIC)
		{
			if(type.isStatic)
			{
				*parseResult = CYMB_PARSE_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_MULTIPLE_STATIC,
					.info = tokens->tokens[0].info
				};

				result = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(result != CYMB_SUCCESS)
				{
					return result;
				}
			}

			type.isStatic = true;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_CONST)
		{
				if(type.isConst)
				{
					*parseResult = CYMB_PARSE_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_MULTIPLE_CONST,
						.info = tokens->tokens[0].info
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(result != CYMB_SUCCESS)
					{
						return result;
					}
				}

			type.isConst = true;
		}

		++tokens->tokens;
		--tokens->count;
		if(tokens->count == 0)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}
	}

	while(tokens->tokens[tokens->count - 1].type == CYMB_TOKEN_CONST)
	{
		if(type.isConst)
		{
			*parseResult = CYMB_PARSE_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_MULTIPLE_CONST,
				.info = tokens->tokens[tokens->count - 1].info
			};

			result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(result != CYMB_SUCCESS)
			{
				return result;
			}
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

		default:
			*parseResult = CYMB_PARSE_INVALID;
			return result;
	}

	info = tokens->tokens[0].info;

	append:
	const CymbNode node = {
		.type = CYMB_NODE_TYPE,
		.typeNode = type,
		.info = info
	};
	return cymbAddNode(builder, &node);
}

/*
 * Parse statement block.
 *
 * Parameters:
 * - builder: A tree builder.
 * - tokens: The tokens to parse.
 * - parseResult: The parse result.
 * - nodes: The node list to store statements.
 * - diagnostics: A list of diagnostics.
 */
static CymbResult cymbParseBlock(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbNodeList* const nodes, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	// Single statement.
	if(tokens->tokens->type != CYMB_TOKEN_OPEN_BRACE)
	{
		result = cymbParseStatement(builder, tokens, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			goto end;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		nodes->count = 1;

		goto commit;
	}

	nodes->nodes = nullptr;
	nodes->count = 0;

	// Block.
	size_t braceCount = 1;
	const CymbToken* endBrace = tokens->tokens + 1;
	while(endBrace < tokens->tokens + tokens->count)
	{
		braceCount += endBrace->type == CYMB_TOKEN_OPEN_BRACE;
		braceCount -= endBrace->type == CYMB_TOKEN_CLOSE_BRACE;

		if(braceCount == 0)
		{
			break;
		}

		++endBrace;
	}
	if(endBrace == tokens->tokens + tokens->count)
	{
		*parseResult = CYMB_PARSE_INVALID;
		result = CYMB_ERROR_INVALID_ARGUMENT;

		goto end;
	}

	const size_t blockSize = endBrace - tokens->tokens - 1;
	CymbConstTokenList blockTokens = {
		.tokens = tokens->tokens + 1,
		.count = blockSize
	};

	while(blockTokens.count > 0)
	{
		result = cymbParseStatement(builder, &blockTokens, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			goto end;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		++nodes->count;
	}

	tokens->tokens = endBrace + 1;
	tokens->count -= blockSize + 2;

	if(nodes->count > 0)
	{
		commit:
		nodes->nodes = builder->tree->children + builder->childCount;
		cymbCommitChildren(builder, nodes->count);
	}

	end:
	return result;
}

static CymbResult cymbParseDeclaration(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	if(!cymbIsKeyword(tokens->tokens[0].type))
	{
		*parseResult = CYMB_PARSE_NO_MATCH;
		return result;
	}

	const CymbToken* equalToken = tokens->tokens;
	while(equalToken < tokens->tokens + tokens->count)
	{
		if(equalToken->type == CYMB_TOKEN_EQUAL)
		{
			break;
		}

		++equalToken;
	}
	const bool hasInitializer = equalToken < tokens->tokens + tokens->count;

	if(hasInitializer && equalToken == tokens->tokens + tokens->count - 1)
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
	if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
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
			.count = tokens->tokens + tokens->count - equalToken - 1
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
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
	return cymbAddNode(builder, &node);
}

CymbResult cymbParseStatement(CymbTreeBuilder* const builder, CymbConstTokenList* const tokens, CymbParseResult* const parseResult, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	if(tokens->tokens[0].type == CYMB_TOKEN_WHILE)
	{
		CymbNode node = {
			.type = CYMB_NODE_WHILE,
			.info = tokens->tokens[0].info
		};

		++tokens->tokens;
		--tokens->count;

		if(tokens->tokens[0].type != CYMB_TOKEN_OPEN_PARENTHESIS)
		{
			*parseResult = CYMB_PARSE_INVALID;
			return result;
		}

		size_t endTokenIndex = 0;
		result = cymbSkipParentheses(tokens, CYMB_DIRECTION_FORWARD, parseResult, &endTokenIndex, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			return result;
		}
		if(endTokenIndex == 1)
		{
			*parseResult = CYMB_PARSE_INVALID;
			result = CYMB_ERROR_INVALID_ARGUMENT;
			return result;
		}

		result = cymbParseExpression(builder, &(CymbConstTokenList){
			.tokens = tokens->tokens + 1,
			.count = endTokenIndex - 1
		}, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			return result;
		}

		node.whileNode.expression = builder->tree->nodes + builder->tree->count - 1;

		tokens->tokens += endTokenIndex + 1;
		tokens->count -= endTokenIndex + 1;

		result = cymbParseBlock(builder, tokens, parseResult, &node.whileNode.body, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			return result;
		}

		result = cymbAddNode(builder, &node);

		return result;
	}

	const CymbToken* endToken = tokens->tokens;
	while(endToken < tokens->tokens + tokens->count)
	{
		if(endToken->type == CYMB_TOKEN_SEMICOLON)
		{
			break;
		}

		++endToken;
	}
	if(endToken == tokens->tokens + tokens->count || tokens->count <= 1)
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

	result = cymbParseDeclaration(builder, &(CymbConstTokenList){
		.tokens = tokens->tokens,
		.count = endToken - tokens->tokens
	}, parseResult, diagnostics);
	if(result != CYMB_SUCCESS || *parseResult == CYMB_PARSE_INVALID)
	{
		return result;
	}
	if(*parseResult == CYMB_PARSE_MATCH)
	{
		goto append;
	}

	result = cymbParseExpression(builder, &(CymbConstTokenList){
		.tokens = tokens->tokens,
		.count = endToken - tokens->tokens
	}, parseResult, diagnostics);
	if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
	{
		return result;
	}

	append:
	*parseResult = CYMB_PARSE_MATCH;

	tokens->count -= endToken + 1 - tokens->tokens;
	tokens->tokens = endToken + 1;

	return result;
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
			.parameterTypes = {
				.nodes = parameterCount == 0 ? nullptr : builder->tree->children + builder->childCount,
				.count = parameterCount
			}
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

	CymbNode node = {
		.type = CYMB_NODE_FUNCTION,
		.functionNode = {
			.name = nameNodePointer,
			.type = typeNodePointer,
			.parameters = {
				.nodes = parameterCount == 0 ? nullptr : parametersStart,
				.count = parameterCount
			}
		},
		.info = nameNode.info
	};

	result = cymbParseBlock(builder, tokens, parseResult, &node.functionNode.statements, diagnostics);
	if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
	{
		result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
		return result;
	}

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

	CymbNode node = {
		.type = CYMB_NODE_PROGRAM,
		.programNode = {
			.children = {
				.nodes = nullptr,
				.count = 0
			}
		}
	};

	while(tokens->count > 0)
	{
		result = cymbParseFunction(builder, tokens, parseResult, diagnostics);
		if(result != CYMB_SUCCESS || *parseResult != CYMB_PARSE_MATCH)
		{
			result = result == CYMB_SUCCESS ? CYMB_ERROR_INVALID_ARGUMENT : result;
			return result;
		}

		result = cymbStoreChild(builder);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}

		++node.programNode.children.count;
	}

	if(node.programNode.children.count > 0)
	{
		node.programNode.children.nodes = builder->tree->children + builder->childCount;
		cymbCommitChildren(builder, node.programNode.children.count);
	}

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
	CYMB_FREE(tree->nodes);
	CYMB_FREE(tree->children);
	tree->count = 0;
}
