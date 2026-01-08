#include "cymb/tree.h"

#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"

/*
 * Create a child node from the last node added and append it to the children list.
 *
 * Parameters:
 * - tree: The tree.
 * - child: The last child in the list.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if the child could not be created.
 */
static CymbResult cymbCreateChild(CymbTree* const tree, CymbNodeChild** const child)
{
	CymbNodeChild* const newChild = cymbArenaAllocate(tree->arena, sizeof(*newChild), alignof(typeof(*newChild)));
	if(!newChild)
	{
		return CYMB_OUT_OF_MEMORY;
	}

	*newChild = (CymbNodeChild){
		.node = tree->root
	};

	if(*child)
	{
		(*child)->next = newChild;
	}
	
	*child = newChild;

	return CYMB_SUCCESS;
}

/*
 * Add a node to a tree.
 *
 * Parameters:
 * - tree: The tree.
 * - node: The node.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if the node could not be added.
 */
static CymbResult cymbAddNode(CymbTree* const tree, const CymbNode* const node)
{
	tree->root = cymbArenaAllocate(tree->arena, sizeof(*node), alignof(typeof(*node)));
	if(!tree->root)
	{
		return CYMB_OUT_OF_MEMORY;
	}

	memcpy(tree->root, node, sizeof(*node));

	return CYMB_SUCCESS;
}

CymbResult cymbSkipParentheses(const CymbTokenList* const tokens, const CymbDirection direction, size_t* const tokenIndex, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	const CymbTokenType plusType = direction == CYMB_DIRECTION_FORWARD ? CYMB_TOKEN_OPEN_PARENTHESIS : CYMB_TOKEN_CLOSE_PARENTHESIS;
	const CymbTokenType minusType = direction == CYMB_DIRECTION_FORWARD ? CYMB_TOKEN_CLOSE_PARENTHESIS : CYMB_TOKEN_OPEN_PARENTHESIS;

	const size_t startIndex = *tokenIndex;
	CymbTokenType type = tokens->tokens[startIndex].type;

	if(type == minusType)
	{
		goto error;
	}

	if(type != plusType)
	{
		result = CYMB_NO_MATCH;
		goto end;
	}

	size_t parenthesisCount = 1;
	while(direction == CYMB_DIRECTION_FORWARD ? *tokenIndex < tokens->count - 1 : *tokenIndex > 0)
	{
		*tokenIndex += direction;

		type = tokens->tokens[*tokenIndex].type;

		parenthesisCount += type == plusType;
		parenthesisCount -= type == minusType;

		if(parenthesisCount == 0)
		{
			goto end;
		}
	}

	error:
	result = CYMB_INVALID;

	const CymbDiagnostic diagnostic = {
		.type = CYMB_UNMATCHED_PARENTHESIS,
		.info = tokens->tokens[startIndex].info
	};

	const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
	if(diagnosticResult != CYMB_SUCCESS)
	{
		result = diagnosticResult;
	}

	end:
	return result;
}

/*
 * An operator associativity.
 */
typedef enum CymbAssociativity
{
	CYMB_LEFT_TO_RIGHT,
	CYMB_RIGHT_TO_LEFT
} CymbAssociativity;

/*
 * A mapping from a token to a binary operator.
 *
 * Fields:
 * - token: The token.
 * - operator: The operator.
 * - precedence: The precedence of the operator.
 * - associativity: The associativity of the operator.
 */
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

/*
 * Compare two binary operators.
 *
 * Parameters:
 * - tokenVoid: A token type.
 * - operatorVoid: A binary operator mapping.
 *
 * Returns:
 * - true if the operators are the same.
 * - false if they are different.
 */
static int cymbCompareBinaryOperator(const void* const tokenVoid, const void* const operatorVoid)
{
	const CymbTokenType token = *(const CymbTokenType*)tokenVoid;
	const CymbBinaryOperatorMapping* operator = operatorVoid;

	return token != operator->token;
}

/*
 * A mapping from a token to a unary operator.
 *
 * Fields:
 * - token: The token.
 * - operator: The operator.
 */
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

/*
 * Compare two unary operators.
 *
 * Parameters:
 * - tokenVoid: A token type.
 * - operatorVoid: A unary operator mapping.
 *
 * Returns:
 * - true if the operators are the same.
 * - false if they are different.
 */
static int cymbCompareUnaryOperator(const void* const tokenVoid, const void* const operatorVoid)
{
	const CymbTokenType token = *(const CymbTokenType*)tokenVoid;
	const CymbUnaryOperatorMapping* operator = operatorVoid;

	return token != operator->token;
}

/*
 * Parse a subexpression.
 *
 * Parameters:
 * - tree: The tree.
 * - tokens: The tokens.
 * - diagnostics: A list of diagnostics.
 * - minimumPrecedence: The minimum precedence to consider the subexpression finished.
 * - hadParenthesis: A flag indicating if seeing a closed parenthesis is valid.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
static CymbResult cymbParseSubexpression(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics, const unsigned char minimumPrecedence, const bool hadParenthesis)
{
	CymbResult result = CYMB_SUCCESS;

	if(tokens->count == 0)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_EXPRESSION,
			.info = (tokens->tokens - 1)->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	if(tokens->tokens[0].type == CYMB_TOKEN_OPEN_PARENTHESIS)
	{
		const CymbToken* const parenthesisToken = tokens->tokens;

		++tokens->tokens;
		--tokens->count;

		result = cymbParseSubexpression(tree, tokens, diagnostics, 0, true);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		if(tokens->count == 0)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNMATCHED_PARENTHESIS,
				.info = parenthesisToken->info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
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

			result = cymbParseSubexpression(tree, tokens, diagnostics, binaryOperators[0].precedence + 1, hadParenthesis);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			node.unaryOperatorNode.node = tree->root;

			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}
		}
		else
		{
			if(tokens->tokens[0].type != CYMB_TOKEN_CONSTANT && tokens->tokens[0].type != CYMB_TOKEN_IDENTIFIER)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_UNEXPECTED_TOKEN,
					.info = tokens->tokens[0].info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				goto end;
			}

			CymbNode leftNode = {
				.type = tokens->tokens[0].type == CYMB_TOKEN_CONSTANT ? CYMB_NODE_CONSTANT : CYMB_NODE_IDENTIFIER,
				.info = tokens->tokens[0].info
			};

			if(tokens->tokens[0].type == CYMB_TOKEN_CONSTANT)
			{
				leftNode.constantNode = tokens->tokens[0].constant;
			}

			result = cymbAddNode(tree, &leftNode);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			++tokens->tokens;
			--tokens->count;
		}
	}

	while(tokens->count > 0)
	{
		if(tokens->tokens[0].type == CYMB_TOKEN_CLOSE_PARENTHESIS)
		{
			if(!hadParenthesis)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_UNMATCHED_PARENTHESIS,
					.info = tokens->tokens[0].info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}
			}

			goto end;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_OPEN_BRACKET)
		{
			const CymbDiagnosticInfo bracketInfo = tokens->tokens[0].info;

			++tokens->tokens;
			--tokens->count;

			CymbNode node = {
				.type = CYMB_NODE_ARRAY_SUBSCRIPT,
				.info = tree->root->info,
				.arraySubscriptNode = {
					.name = tree->root
				}
			};

			CymbToken* token = tokens->tokens;

			size_t parenthesisCount = 0;
			size_t bracketCount = 0;
			while(
				token < tokens->tokens + tokens->count &&
				!(parenthesisCount == 0 && bracketCount == 0 && token->type == CYMB_TOKEN_CLOSE_BRACKET)
			)
			{
				parenthesisCount += token->type == CYMB_TOKEN_OPEN_PARENTHESIS;
				parenthesisCount -= token->type == CYMB_TOKEN_CLOSE_PARENTHESIS;

				bracketCount += token->type == CYMB_TOKEN_OPEN_BRACKET;
				bracketCount -= token->type == CYMB_TOKEN_CLOSE_BRACKET;

				++token;
			}
			if(token == tokens->tokens + tokens->count)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_UNMATCHED_BRACKET,
					.info = bracketInfo
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				goto end;
			}

			const size_t argumentLength = token - tokens->tokens;

			if(argumentLength == 0)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_EXPECTED_EXPRESSION,
					.info = token->info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				goto end;
			}

			result = cymbParseSubexpression(tree, &(CymbTokenList){
				.tokens = tokens->tokens,
				.count = argumentLength
			}, diagnostics, 0, false);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			node.arraySubscriptNode.expression = tree->root;

			tokens->tokens = token + 1;
			tokens->count -= argumentLength + 1;

			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			continue;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_OPEN_PARENTHESIS)
		{
			const CymbDiagnosticInfo parenthesisInfo = tokens->tokens[0].info;

			++tokens->tokens;
			--tokens->count;

			CymbNode node = {
				.type = CYMB_NODE_FUNCTION_CALL,
				.info = tree->root->info,
				.functionCallNode = {
					.name = tree->root
				}
			};

			CymbNodeChild* arguments = nullptr;

			while(true)
			{
				CymbToken* token = tokens->tokens;

				size_t parenthesisCount = 0;
				while(
					token < tokens->tokens + tokens->count &&
					!(parenthesisCount == 0 && token->type == CYMB_TOKEN_COMMA) &&
					!(parenthesisCount == 0 && token->type == CYMB_TOKEN_CLOSE_PARENTHESIS)
				)
				{
					parenthesisCount += token->type == CYMB_TOKEN_OPEN_PARENTHESIS;
					parenthesisCount -= token->type == CYMB_TOKEN_CLOSE_PARENTHESIS;

					++token;
				}
				if(token == tokens->tokens + tokens->count)
				{
					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_UNMATCHED_PARENTHESIS,
						.info = parenthesisInfo
					};
					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
				}

				const size_t argumentLength = token - tokens->tokens;

				if(argumentLength == 0)
				{
					if(!node.functionCallNode.arguments)
					{
						++tokens->tokens;
						--tokens->count;

						break;
					}

					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_EXPECTED_EXPRESSION,
						.info = token->info
					};
					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
				}

				result = cymbParseSubexpression(tree, &(CymbTokenList){
					.tokens = tokens->tokens,
					.count = argumentLength
				}, diagnostics, 0, false);
				if(result != CYMB_SUCCESS)
				{
					goto end;
				}

				result = cymbCreateChild(tree, &arguments);
				if(result != CYMB_SUCCESS)
				{
					goto end;
				}

				if(!node.functionCallNode.arguments)
				{
					node.functionCallNode.arguments = arguments;
				}

				tokens->tokens = token + 1;
				tokens->count -= argumentLength + 1;

				if(tokens->tokens[-1].type == CYMB_TOKEN_CLOSE_PARENTHESIS)
				{
					break;
				}
			}

			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			continue;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_DOT || tokens->tokens[0].type == CYMB_TOKEN_ARROW)
		{
			if(tokens->count <= 1 || tokens->tokens[1].type != CYMB_TOKEN_IDENTIFIER)
			{
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_EXPECTED_IDENTIFIER,
					.info = tokens->tokens[0].info
				};
				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				goto end;
			}

			CymbNode* const name = tree->root;

			CymbNode node = {
				.type = CYMB_NODE_IDENTIFIER,
				.info = tokens->tokens[1].info
			};
			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			node = (CymbNode){
				.type = CYMB_NODE_MEMBER_ACCESS,
				.memberAccessNode = {
					.type = tokens->tokens[0].type == CYMB_TOKEN_DOT ? CYMB_MEMBER_ACCESS : CYMB_MEMBER_ACCESS_POINTER,
					.name = name,
					.member = tree->root
				},
				.info = tokens->tokens[0].info
			};
			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			tokens->tokens += 2;
			tokens->count -= 2;

			continue;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_PLUS_PLUS || tokens->tokens[0].type == CYMB_TOKEN_MINUS_MINUS)
		{
			const CymbNode node = {
				.type = CYMB_NODE_POSTFIX_OPERATOR,
				.postfixOperatorNode = {
					.operator = tokens->tokens[0].type == CYMB_TOKEN_PLUS_PLUS ? CYMB_POSTFIX_OPERATOR_INCREMENT : CYMB_POSTFIX_OPERATOR_DECREMENT,
					.node = tree->root
				},
				.info = tokens->tokens[0].info
			};

			result = cymbAddNode(tree, &node);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			++tokens->tokens;
			--tokens->count;

			continue;
		}

		const CymbBinaryOperatorMapping* const operator = cymbFind(&tokens->tokens[0].type, binaryOperators, binaryOperatorCount, binaryOperatorSize, cymbCompareBinaryOperator);
		if(!operator)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNEXPECTED_TOKEN,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		if(operator->precedence < minimumPrecedence || (operator->associativity == CYMB_LEFT_TO_RIGHT && operator->precedence == minimumPrecedence))
		{
			goto end;
		}

		CymbNode operatorNode = {
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tokens->tokens[0].info,
			.binaryOperatorNode = {
				.operator = operator->operator,
				.leftNode = tree->root
			}
		};

		++tokens->tokens;
		--tokens->count;

		result = cymbParseSubexpression(tree, tokens, diagnostics, operator->precedence, hadParenthesis);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		operatorNode.binaryOperatorNode.rightNode = tree->root;

		result = cymbAddNode(tree, &operatorNode);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}
	}

	end:
	return result;
}

CymbResult cymbParseExpression(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	return cymbParseSubexpression(tree, tokens, diagnostics, 0, false);
}

CymbResult cymbParseType(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
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
						result = CYMB_INVALID;

						const CymbDiagnostic diagnostic = {
							.type = CYMB_MULTIPLE_CONST,
							.info = tokens->tokens[qualifierIndex].info
						};

						const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
						if(diagnosticResult != CYMB_SUCCESS)
						{
							result = diagnosticResult;
						}

						goto end;
					}

					pointer.isConst = true;
				}

				else if(tokens->tokens[qualifierIndex].type == CYMB_TOKEN_RESTRICT)
				{
					if(pointer.isRestrict)
					{
						result = CYMB_INVALID;

						const CymbDiagnostic diagnostic = {
							.type = CYMB_MULTIPLE_RESTRICT,
							.info = tokens->tokens[qualifierIndex].info
						};

						const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
						if(diagnosticResult != CYMB_SUCCESS)
						{
							result = diagnosticResult;
						}

						goto end;
					}

					pointer.isRestrict = true;
				}

				else
				{
					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_UNEXPECTED_TOKEN,
						.info = tokens->tokens[qualifierIndex].info
					};

					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
				}
			}

			result = cymbParseType(tree, &(CymbTokenList){
				.tokens = tokens->tokens,
				.count = pointerIndex
			}, diagnostics);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}

			pointer.pointedNode = tree->root;

			const CymbNode node = {
				.type = CYMB_NODE_POINTER,
				.pointerNode = pointer,
				.info = tokens->tokens[pointerIndex].info
			};
			result = cymbAddNode(tree, &node);

			goto end;
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
				result = CYMB_INVALID;

				const CymbDiagnostic diagnostic = {
					.type = CYMB_MULTIPLE_STATIC,
					.info = tokens->tokens[0].info
				};

				const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
				if(diagnosticResult != CYMB_SUCCESS)
				{
					result = diagnosticResult;
				}

				goto end;
			}

			type.isStatic = true;
		}

		if(tokens->tokens[0].type == CYMB_TOKEN_CONST)
		{
				if(type.isConst)
				{
					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_MULTIPLE_CONST,
						.info = tokens->tokens[0].info
					};

					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
				}

			type.isConst = true;
		}

		++tokens->tokens;
		--tokens->count;

		if(tokens->count == 0)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_MISSING_TYPE,
				.info = (tokens->tokens - 1)->info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}
	}

	while(tokens->tokens[tokens->count - 1].type == CYMB_TOKEN_CONST)
	{
		if(type.isConst)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_MULTIPLE_CONST,
				.info = tokens->tokens[tokens->count - 1].info
			};

			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		type.isConst = true;

		--tokens->count;

		if(tokens->count == 0)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_MISSING_TYPE,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}
	}

	if(tokens->count == 3)
	{
		if(tokens->tokens[1].type != CYMB_TOKEN_LONG)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_TYPE,
				.info = tokens->tokens[1].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
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

		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_TYPE,
			.info = tokens->tokens[1].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
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
					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_INVALID_TYPE,
						.info = tokens->tokens[typeIndex].info
					};
					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
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
					result = CYMB_INVALID;

					const CymbDiagnostic diagnostic = {
						.type = CYMB_INVALID_TYPE,
						.info = tokens->tokens[typeIndex].info
					};
					const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
					if(diagnosticResult != CYMB_SUCCESS)
					{
						result = diagnosticResult;
					}

					goto end;
			}

			info = tokens->tokens[typeIndex].info;

			goto append;
		}

		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_TYPE,
			.info = tokens->tokens[1].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	if(tokens->count != 1)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_TYPE,
			.info = tokens->tokens[0].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
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
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_TYPE,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
	}

	info = tokens->tokens[0].info;

	append:
	const CymbNode node = {
		.type = CYMB_NODE_TYPE,
		.typeNode = type,
		.info = info
	};
	result = cymbAddNode(tree, &node);

	end:
	return result;
}

/*
 * Parse statement block.
 *
 * Parameters:
 * - tree: A tree.
 * - tokens: The tokens to parse.
 * - children: The child nodes of the block.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
static CymbResult cymbParseBlock(CymbTree* const tree, CymbTokenList* const tokens, CymbNodeChild** const children, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*children = nullptr;
	CymbNodeChild* childrenList = nullptr;

	// Single statement.
	if(tokens->tokens->type != CYMB_TOKEN_OPEN_BRACE)
	{
		result = cymbParseStatement(tree, tokens, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbCreateChild(tree, children);

		goto end;
	}

	// Block.
	size_t braceCount = 1;
	CymbToken* endBrace = tokens->tokens + 1;
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
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNMATCHED_BRACE,
			.info = tokens->tokens[0].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	const size_t blockSize = endBrace - tokens->tokens - 1;
	CymbTokenList blockTokens = {
		.tokens = tokens->tokens + 1,
		.count = blockSize
	};

	while(blockTokens.count > 0)
	{
		result = cymbParseStatement(tree, &blockTokens, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbCreateChild(tree, &childrenList);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		if(!*children)
		{
			*children = childrenList;
		}
	}

	tokens->tokens = endBrace + 1;
	tokens->count -= blockSize + 2;

	end:
	return result;
}

/*
 * Parse a declaration.
 *
 * Parameters:
 * - tree: A tree.
 * - tokens: The tokens to parse.
 * - diagnostics: A list of diagnostics.
 *
 * Retuns:
 * - CYMB_SUCCESS on success.
 * - CYMB_NO_MATCH if there is no match.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
static CymbResult cymbParseDeclaration(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	if(!cymbIsKeyword(tokens->tokens[0].type))
	{
		result = CYMB_NO_MATCH;
		goto end;
	}

	CymbToken* equalToken = tokens->tokens;
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
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_EXPRESSION,
			.info = equalToken->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	if(equalToken < tokens->tokens + 2)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_DECLARATION,
			.info = equalToken->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	CymbToken* const identifierToken = equalToken - 1;
	if(identifierToken->type != CYMB_TOKEN_IDENTIFIER)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_DECLARATION,
			.info = identifierToken->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	CymbToken* typeToken = identifierToken;
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
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNEXPECTED_TOKEN,
				.info = typeToken->info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}
	} while(typeToken > tokens->tokens);

	result = cymbParseType(tree, &(CymbTokenList){
		.tokens = tokens->tokens,
		.count = equalToken - tokens->tokens - 1
	}, diagnostics);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}
	CymbNode* const typeNode = tree->root;

	const CymbNode identifierNode = {
		.type = CYMB_NODE_IDENTIFIER,
		.info = identifierToken->info
	};
	result = cymbAddNode(tree, &identifierNode);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}
	CymbNode* const identifierNodePointer = tree->root;

	if(hasInitializer)
	{
		result = cymbParseExpression(tree, &(CymbTokenList){
			.tokens = equalToken + 1,
			.count = tokens->tokens + tokens->count - equalToken - 1
		}, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}
	}

	const CymbNode node = {
		.type = CYMB_NODE_DECLARATION,
		.declarationNode = {
			.identifier = identifierNodePointer,
			.type = typeNode,
			.initializer = hasInitializer ? tree->root : nullptr
		},
		.info = identifierToken->info
	};
	result = cymbAddNode(tree, &node);

	end:
	return result;
}

CymbResult cymbParseStatement(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
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

		if(tokens->count == 0 || tokens->tokens[0].type != CYMB_TOKEN_OPEN_PARENTHESIS)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_EXPECTED_PARENTHESIS,
				.info = (tokens->tokens - 1)->info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		size_t endTokenIndex = 0;
		result = cymbSkipParentheses(tokens, CYMB_DIRECTION_FORWARD, &endTokenIndex, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			return result;
		}
		if(endTokenIndex == 1)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_EXPECTED_EXPRESSION,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		result = cymbParseExpression(tree, &(CymbTokenList){
			.tokens = tokens->tokens + 1,
			.count = endTokenIndex - 1
		}, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		node.whileNode.expression = tree->root;

		tokens->tokens += endTokenIndex + 1;
		tokens->count -= endTokenIndex + 1;

		result = cymbParseBlock(tree, tokens, &node.whileNode.body, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbAddNode(tree, &node);

		goto end;
	}

	if(tokens->count == 1 && tokens->tokens[0].type == CYMB_TOKEN_SEMICOLON)
	{
		goto end;
	}

	CymbToken* endToken = tokens->tokens;
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
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_SEMICOLON,
			.info = (endToken - 1)->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	if(tokens->tokens[0].type == CYMB_TOKEN_RETURN)
	{
		const bool empty = endToken == tokens->tokens + 1;

		if(!empty)
		{
			result = cymbParseExpression(tree, &(CymbTokenList){
				.tokens = tokens->tokens + 1,
				.count = endToken - tokens->tokens - 1
			}, diagnostics);
			if(result != CYMB_SUCCESS)
			{
				goto end;
			}
		}

		const CymbNode node = {
			.type = CYMB_NODE_RETURN,
			.returnNode = empty ? nullptr : tree->root,
			.info = tokens->tokens[0].info
		};
		result = cymbAddNode(tree, &node);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		goto append;
	}

	result = cymbParseDeclaration(tree, &(CymbTokenList){
		.tokens = tokens->tokens,
		.count = endToken - tokens->tokens
	}, diagnostics);
	if(result == CYMB_SUCCESS)
	{
		goto append;
	}
	if(result != CYMB_NO_MATCH)
	{
		goto end;
	}

	result = cymbParseExpression(tree, &(CymbTokenList){
		.tokens = tokens->tokens,
		.count = endToken - tokens->tokens
	}, diagnostics);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	append:
	tokens->count -= endToken + 1 - tokens->tokens;
	tokens->tokens = endToken + 1;

	end:
	return result;
}

CymbResult cymbParseFunction(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	CymbNode typeNode = {
		.type = CYMB_NODE_FUNCTION_TYPE
	};

	CymbNode node = {
		.type = CYMB_NODE_FUNCTION
	};

	CymbNodeChild* parameterTypes = nullptr;
	CymbNodeChild* parameters = nullptr;

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
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_FUNCTION,
			.info = tokens->tokens[0].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	--name;
	if(name->type != CYMB_TOKEN_IDENTIFIER)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_FUNCTION,
			.info = tokens->tokens[0].info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	// Parse return type.
	const size_t typeCount = name - tokens->tokens;

	result = cymbParseType(tree, &(CymbTokenList){
		.tokens = tokens->tokens,
		.count = typeCount
	}, diagnostics);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	typeNode.functionTypeNode.returnType = tree->root;
	typeNode.info = typeNode.functionTypeNode.returnType->info;

	// Parse parameters.
	const size_t parametersOffset = typeCount + 2;
	tokens->count -= parametersOffset;
	tokens->tokens += parametersOffset;
	if(tokens->count == 0)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = (tokens->tokens - 1)->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

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
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_EXPECTED_PARAMETER,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		--parameterName;
		if(parameterName->type != CYMB_TOKEN_IDENTIFIER)
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_EXPECTED_PARAMETER,
				.info = tokens->tokens[0].info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		const size_t parameterTypeCount = parameterName - tokens->tokens;
		result = cymbParseType(tree, &(CymbTokenList){
			.tokens = tokens->tokens,
			.count = parameterTypeCount
		}, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbCreateChild(tree, &parameterTypes);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		if(!typeNode.functionTypeNode.parameterTypes)
		{
			typeNode.functionTypeNode.parameterTypes = parameterTypes;
		}

		const CymbNode parameterNameNode = {
			.type = CYMB_NODE_IDENTIFIER,
			.info = parameterName->info
		};
		result = cymbAddNode(tree, &parameterNameNode);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbCreateChild(tree, &parameters);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		if(!node.functionNode.parameters)
		{
			node.functionNode.parameters = parameters;
		}

		const size_t parameterOffset = parameterTypeCount + 1 + ((parameterName + 1)->type == CYMB_TOKEN_COMMA);
		tokens->count -= parameterOffset;
		tokens->tokens += parameterOffset;
	}

	if(tokens->count == 0)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_FUNCTION,
			.info = (tokens->tokens - 1)->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	result = cymbAddNode(tree, &typeNode);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}
	node.functionNode.type = tree->root;

	const CymbNode nameNode = {
		.type = CYMB_NODE_IDENTIFIER,
		.info = name->info
	};
	result = cymbAddNode(tree, &nameNode);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}
	node.functionNode.name = tree->root;
	node.info = node.functionNode.name->info;

	++tokens->tokens;
	--tokens->count;

	// Parse statements.
	if(tokens->count == 0 || tokens->tokens->type != CYMB_TOKEN_OPEN_BRACE)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_EXPECTED_FUNCTION,
			.info = (tokens->tokens - 1)->info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	result = cymbParseBlock(tree, tokens, &node.functionNode.statements, diagnostics);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	result = cymbAddNode(tree, &node);

	end:
	return result;
}

CymbResult cymbParseProgram(CymbTree* const tree, CymbTokenList* const tokens, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	CymbNode node = {
		.type = CYMB_NODE_PROGRAM
	};

	CymbNodeChild* children = nullptr;

	while(tokens->count > 0)
	{
		result = cymbParseFunction(tree, tokens, diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		result = cymbCreateChild(tree, &children);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}

		if(!node.programNode.children)
		{
			node.programNode.children = children;
		}
	}

	result = cymbAddNode(tree, &node);
	if(result != CYMB_SUCCESS)
	{
		goto end;
	}

	result = tokens->count == 0 ? CYMB_SUCCESS : CYMB_INVALID;

	end:
	return result;
}

CymbResult cymbParse(const CymbTokenList* const tokens, CymbArena* const arena, CymbTree* const tree, CymbDiagnosticList* const diagnostics)
{
	tree->root = nullptr;
	tree->arena = arena;

	CymbResult result = cymbParseProgram(tree, &(CymbTokenList){
		.tokens = tokens->tokens,
		.count = tokens->count
	}, diagnostics);
	if(result != CYMB_SUCCESS)
	{
		goto error;
	}

	goto end;

	error:
	cymbFreeTree(tree);

	end:
	return result;
}

void cymbFreeTree(CymbTree* const tree)
{
	*tree = (CymbTree){};
}
