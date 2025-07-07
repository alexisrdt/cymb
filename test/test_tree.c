#include "test.h"

#include <stdio.h>

#include "cymb/tree.h"

typedef struct CymbTreeTest
{
	CymbConstTokenList tokens;
	CymbParseResult result;
	CymbTreeBuilder solution;
	CymbConstDiagnosticList diagnostics;
	size_t endTokensOffset;
} CymbTreeTest;

static bool cymbComparePointers(const void* const first, const void* const firstRef, const void* const second, const void* const secondRef)
{
	return (!first && !second) || (first && second && (const char*)first - (const char*)firstRef == (const char*)second - (const char*)secondRef);
}

static void cymbCompareTrees(const CymbTreeBuilder* const first, const CymbTreeBuilder* const second, CymbTestContext* const context)
{
	if(first->tree->count != second->tree->count)
	{
		cymbFail(context, "Wrong node count.");
		return;
	}

	if(first->childCount != second->childCount)
	{
		cymbFail(context, "Wrong child count.");
		return;
	}

	if(first->lastStored != first->childrenCapacity || second->lastStored != second->childrenCapacity)
	{
		cymbFail(context, "Wrong last stored.");
	}

	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	for(size_t childIndex = 0; childIndex < first->childCount; ++childIndex)
	{
		snprintf(buffer, sizeof(buffer), format, "child", childIndex);

		if(!cymbComparePointers(first->tree->children[childIndex], first->tree->nodes, second->tree->children[childIndex], second->tree->nodes))
		{
			cymbFail(context, "Wrong child.");
		}
	}

	for(size_t nodeIndex = 0; nodeIndex < first->tree->count; ++nodeIndex)
	{
		snprintf(buffer, sizeof(buffer), format, "node", nodeIndex);

		CymbNode* const firstNode = &first->tree->nodes[nodeIndex];
		CymbNode* const secondNode = &second->tree->nodes[nodeIndex];

		if(firstNode->type != secondNode->type)
		{
			cymbFail(context, "Wrong node type.");
			continue;
		}

		cymbCompareDiagnosticInfo(&firstNode->info, &secondNode->info, context);

		switch(firstNode->type)
		{
			case CYMB_NODE_CONSTANT:
				if(firstNode->constantNode.type != secondNode->constantNode.type || firstNode->constantNode.value != secondNode->constantNode.value)
				{
					cymbFail(context, "Wrong constant.");
				}
				break;

			case CYMB_NODE_IDENTIFIER:
				break;

			case CYMB_NODE_BINARY_OPERATOR:
				if(firstNode->constantNode.type != secondNode->constantNode.type)
				{
					cymbFail(context, "Wrong binary operator type.");
				}
				if(!cymbComparePointers(firstNode->binaryOperatorNode.leftNode, first->tree->nodes, secondNode->binaryOperatorNode.leftNode, second->tree->nodes))
				{
					cymbFail(context, "Wrong binary operator left node.");
				}
				if(!cymbComparePointers(firstNode->binaryOperatorNode.rightNode, first->tree->nodes, secondNode->binaryOperatorNode.rightNode, second->tree->nodes))
				{
					cymbFail(context, "Wrong binary operator right node.");
				}
				break;

			case CYMB_NODE_WHILE:
				if(!cymbComparePointers(firstNode->whileNode.expression, first->tree->nodes, secondNode->whileNode.expression, second->tree->nodes))
				{
					cymbFail(context, "Wrong expression node.");
				}
				if(firstNode->whileNode.body.count != secondNode->whileNode.body.count)
				{
					cymbFail(context, "Wrong body statement count.");
				}
				if(!cymbComparePointers(firstNode->whileNode.body.nodes, first->tree->children, secondNode->whileNode.body.nodes, second->tree->children))
				{
					cymbFail(context, "Wrong body statement pointer.");
				}
				break;

			case CYMB_NODE_RETURN:
				if(!cymbComparePointers(firstNode->returnNode, first->tree->nodes, secondNode->returnNode, second->tree->nodes))
				{
					cymbFail(context, "Wrong return node.");
				}
				break;

			case CYMB_NODE_TYPE:
				if(firstNode->typeNode.type != secondNode->typeNode.type)
				{
					cymbFail(context, "Wrong type type.");
				}
				if(firstNode->typeNode.isConst != secondNode->typeNode.isConst)
				{
					cymbFail(context, "Wrong type const.");
				}
				if(firstNode->typeNode.isStatic != secondNode->typeNode.isStatic)
				{
					cymbFail(context, "Wrong type static.");
				}
				break;

			case CYMB_NODE_POINTER:
				if(!cymbComparePointers(firstNode->pointerNode.pointedNode, first->tree->nodes, secondNode->pointerNode.pointedNode, second->tree->nodes))
				{
					cymbFail(context, "Wrong pointer pointed node.");
				}
				if(firstNode->pointerNode.isConst != secondNode->pointerNode.isConst)
				{
					cymbFail(context, "Wrong pointer const.");
				}
				if(firstNode->pointerNode.isRestrict != secondNode->pointerNode.isRestrict)
				{
					cymbFail(context, "Wrong pointer restrict.");
				}
				break;

			case CYMB_NODE_FUNCTION_TYPE:
				if(!cymbComparePointers(firstNode->functionTypeNode.returnType, first->tree->nodes, secondNode->functionTypeNode.returnType, second->tree->nodes))
				{
					cymbFail(context, "Wrong function type return node.");
				}
				if(firstNode->functionTypeNode.parameterTypes.count != secondNode->functionTypeNode.parameterTypes.count)
				{
					cymbFail(context, "Wrong function type parameter count.");
				}
				if(!cymbComparePointers(firstNode->functionTypeNode.parameterTypes.nodes, first->tree->children, secondNode->functionTypeNode.parameterTypes.nodes, second->tree->children))
				{
					cymbFail(context, "Wrong function type parameter pointer.");
				}
				break;

			case CYMB_NODE_DECLARATION:
				if(!cymbComparePointers(firstNode->declarationNode.identifier, first->tree->nodes, secondNode->declarationNode.identifier, second->tree->nodes))
				{
					cymbFail(context, "Wrong declaration identifier.");
				}
				if(!cymbComparePointers(firstNode->declarationNode.type, first->tree->nodes, secondNode->declarationNode.type, second->tree->nodes))
				{
					cymbFail(context, "Wrong declaration type.");
				}
				if(!cymbComparePointers(firstNode->declarationNode.initializer, first->tree->nodes, secondNode->declarationNode.initializer, second->tree->nodes))
				{
					cymbFail(context, "Wrong declaration initializer.");
				}
				break;

			case CYMB_NODE_FUNCTION:
				if(!cymbComparePointers(firstNode->functionNode.name, first->tree->nodes, secondNode->functionNode.name, second->tree->nodes))
				{
					cymbFail(context, "Wrong function name.");
				}
				if(!cymbComparePointers(firstNode->functionNode.type, first->tree->nodes, secondNode->functionNode.type, second->tree->nodes))
				{
					cymbFail(context, "Wrong function type.");
				}
				if(firstNode->functionNode.parameters.count != secondNode->functionNode.parameters.count)
				{
					cymbFail(context, "Wrong function parameter count.");
				}
				if(!cymbComparePointers(firstNode->functionNode.parameters.nodes, first->tree->children, secondNode->functionNode.parameters.nodes, second->tree->children))
				{
					cymbFail(context, "Wrong function parameters pointer.");
				}
				if(firstNode->functionNode.statements.count != secondNode->functionNode.statements.count)
				{
					cymbFail(context, "Wrong function statement count.");
				}
				if(!cymbComparePointers(firstNode->functionNode.statements.nodes, first->tree->children, secondNode->functionNode.statements.nodes, second->tree->children))
				{
					cymbFail(context, "Wrong function statements pointer.");
				}
				break;

			case CYMB_NODE_PROGRAM:
				if(firstNode->programNode.children.count != secondNode->programNode.children.count)
				{
					cymbFail(context, "Wrong program child count.");
				}
				if(!cymbComparePointers(firstNode->programNode.children.nodes, first->tree->children, secondNode->programNode.children.nodes, second->tree->children))
				{
					cymbFail(context, "Wrong program children pointer.");
				}
				break;

			default:
				cymbFail(context, "Unknown node type.");
				break;
		}
	}

	--context->stringCount;
}

static void cymbDoTreeTest(const CymbTreeTest* const test, const CymbTreeFunction function, CymbTestContext* const context)
{
	context->diagnostics.count = 0;

	CymbNode nodes[256];
	CymbNode* children[256];

	CymbTree tree = {.nodes = nodes, .children = children};
	CymbTreeBuilder builder = {
		.tree = &tree,
		.nodesCapacity = CYMB_LENGTH(nodes),
		.childrenCapacity = CYMB_LENGTH(children),
		.lastStored = CYMB_LENGTH(children)
	};

	CymbConstTokenList tokens = test->tokens;

	CymbParseResult parseResult;
	const CymbResult result = function(&builder, &tokens, &parseResult, &context->diagnostics);
	if(result != CYMB_SUCCESS)
	{
		cymbFail(context, "Parse failed.");
		return;
	}

	if(parseResult != test->result)
	{
		cymbFail(context, "Wrong result.");
		return;
	}

	cymbCompareDiagnostics(&(CymbConstDiagnosticList){
		.diagnostics = context->diagnostics.diagnostics,
		.count = context->diagnostics.count
	}, &test->diagnostics, context);

	if(parseResult != CYMB_PARSE_MATCH)
	{
		return;
	}

	if(test->endTokensOffset > 0 && (tokens.tokens != test->tokens.tokens + test->endTokensOffset || tokens.count != test->tokens.count - test->endTokensOffset))
	{
		cymbFail(context, "Wrong end tokens.");
	}

	cymbCompareTrees(&builder, &test->solution, context);
}

static void cymbTestParentheses(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const struct
	{
		CymbConstTokenList tokens;
		CymbDirection direction;
		CymbParseResult result;
		size_t startIndex;
		size_t endIndex;
		CymbConstDiagnosticList diagnostics;
	} tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_PLUS}
			},
			.count = 1
		}, CYMB_DIRECTION_FORWARD, CYMB_PARSE_NO_MATCH, 0, 0, {}},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {8, 8},
						.line = CYMB_STRING("  \t a (!"),
						.hint = CYMB_STRING("(")
					}
				}
			},
			.count = 1
		}, CYMB_DIRECTION_FORWARD, CYMB_PARSE_INVALID, 0, 0, {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_UNMATCHED_PARENTHESIS,
					.info = tests[1].tokens.tokens[0].info
				}
			}
		}},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 1},
						.line = CYMB_STRING("("),
						.hint = CYMB_STRING("(")
					}
				}
			},
			.count = 1
		}, CYMB_DIRECTION_BACKWARD, CYMB_PARSE_INVALID, 0, 0, {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_UNMATCHED_PARENTHESIS,
					.info = tests[2].tokens.tokens[0].info
				}
			}
		}},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 2
		}, CYMB_DIRECTION_FORWARD, CYMB_PARSE_MATCH, 0, 1, {}},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 2
		}, CYMB_DIRECTION_BACKWARD, CYMB_PARSE_MATCH, 1, 0, {}},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {2, 2},
						.line = CYMB_STRING(" ) "),
						.hint = CYMB_STRING(")")
					}
				},
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 3
		}, CYMB_DIRECTION_FORWARD, CYMB_PARSE_INVALID, 0, 0, {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_UNMATCHED_PARENTHESIS,
					.info = tests[5].tokens.tokens[0].info
				}
			}
		}},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {3, 3},
						.line = CYMB_STRING(" ( "),
						.hint = CYMB_STRING("(")
					}
				}
			},
			.count = 3
		}, CYMB_DIRECTION_BACKWARD, CYMB_PARSE_INVALID, 2, 2, {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_UNMATCHED_PARENTHESIS,
					.info = tests[6].tokens.tokens[2].info
				}
			}
		}},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS}
			},
			.count = 8
		}, CYMB_DIRECTION_FORWARD, CYMB_PARSE_MATCH, 0, 6, {}},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS}, 
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS},
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 8
		}, CYMB_DIRECTION_BACKWARD, CYMB_PARSE_MATCH, 7, 1, {}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		context->diagnostics.count = 0;

		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		size_t tokenIndex = tests[testIndex].startIndex;
		CymbParseResult parseResult;
		if(cymbSkipParentheses(&tests[testIndex].tokens, tests[testIndex].direction, &parseResult, &tokenIndex, &context->diagnostics) != CYMB_SUCCESS)
		{
			cymbFail(context, "Wrong result.");
			continue;
		}

		if(parseResult != tests[testIndex].result)
		{
			cymbFail(context, "Wrong parse result.");
			continue;
		}

		if(parseResult == CYMB_PARSE_MATCH && tokenIndex != tests[testIndex].endIndex)
		{
			cymbFail(context, "Wrong token index.");
			continue;
		}

		if(parseResult == CYMB_PARSE_INVALID)
		{
			cymbCompareDiagnostics(&(const CymbConstDiagnosticList){
				.count = context->diagnostics.count,
				.diagnostics = context->diagnostics.diagnostics
			}, &tests[testIndex].diagnostics, context);
		}
	}

	--context->stringCount;
}

static void cymbTestExpressions(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbConstString test2String = CYMB_STRING("((5 * (26 + 27 * 28 + 29) + 37))");
	const CymbConstString test3String = CYMB_STRING("0 & 1 << 2 == 3 + 4 * 5 || 6 ^ 7 < 8 >> 9 / 10 && 11 - 12 >= 13 != 14 <= 15 | 16 > 17 % 18");

	const CymbTreeTest tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_PLUS},
				{.type = CYMB_TOKEN_CONSTANT, .constant = {CYMB_CONSTANT_INT, 5}}
			},
			.count = 2
		}, CYMB_PARSE_NO_MATCH, {}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_CONSTANT, .constant = {CYMB_CONSTANT_INT, 5}},
				{.type = CYMB_TOKEN_PLUS}
			},
			.count = 2
		}, CYMB_PARSE_NO_MATCH, {}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 1},
						.line = test2String,
						.hint = {test2String.string + 0, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 2},
						.line = test2String,
						.hint = {test2String.string + 1, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 3},
						.line = test2String,
						.hint = {test2String.string + 2, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 5},
						.line = test2String,
						.hint = {test2String.string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 7},
						.line = test2String,
						.hint = {test2String.string + 6, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 26},
					.info = {
						.position = {1, 8},
						.line = test2String,
						.hint = {test2String.string + 7, 2}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 11},
						.line = test2String,
						.hint = {test2String.string + 10, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 27},
					.info = {
						.position = {1, 13},
						.line = test2String,
						.hint = {test2String.string + 12, 2}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 16},
						.line = test2String,
						.hint = {test2String.string + 15, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 28},
					.info = {
						.position = {1, 18},
						.line = test2String,
						.hint = {test2String.string + 17, 2}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 21},
						.line = test2String,
						.hint = {test2String.string + 20, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 29},
					.info = {
						.position = {1, 23},
						.line = test2String,
						.hint = {test2String.string + 22, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 25},
						.line = test2String,
						.hint = {test2String.string + 24, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 27},
						.line = test2String,
						.hint = {test2String.string + 26, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 37},
					.info = {
						.position = {1, 29},
						.line = test2String,
						.hint = {test2String.string + 28, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 31},
						.line = test2String,
						.hint = {test2String.string + 30, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 32},
						.line = test2String,
						.hint = {test2String.string + 31, 1}
					}
				}
			},
			.count = 17
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 5},
						.info = tests[2].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 26},
						.info = tests[2].tokens.tokens[5].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 27},
						.info = tests[2].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 28},
						.info = tests[2].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_MUL},
						.info = tests[2].tokens.tokens[8].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_SUM},
						.info = tests[2].tokens.tokens[6].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 29},
						.info = tests[2].tokens.tokens[11].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_SUM},
						.info = tests[2].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_MUL},
						.info = tests[2].tokens.tokens[3].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 37},
						.info = tests[2].tokens.tokens[14].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_SUM},
						.info = tests[2].tokens.tokens[13].info
					}
				},
				.count = 11
			}
		}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 0},
					.info = {
						.position = {1, 1},
						.line = test3String,
						.hint = {test3String.string + 0, 1}
					}
				},
				{
					.type = CYMB_TOKEN_AMPERSAND,
					.info = {
						.position = {1, 3},
						.line = test3String,
						.hint = {test3String.string + 2, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 5},
						.line = test3String,
						.hint = {test3String.string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_LEFT_SHIFT,
					.info = {
						.position = {1, 7},
						.line = test3String,
						.hint = {test3String.string + 6, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 2},
					.info = {
						.position = {1, 10},
						.line = test3String,
						.hint = {test3String.string + 9, 1}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL_EQUAL,
					.info = {
						.position = {1, 12},
						.line = test3String,
						.hint = {test3String.string + 11, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 3},
					.info = {
						.position = {1, 15},
						.line = test3String,
						.hint = {test3String.string + 14, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 17},
						.line = test3String,
						.hint = {test3String.string + 16, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 4},
					.info = {
						.position = {1, 19},
						.line = test3String,
						.hint = {test3String.string + 18, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 21},
						.line = test3String,
						.hint = {test3String.string + 20, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 23},
						.line = test3String,
						.hint = {test3String.string + 22, 1}
					}
				},
				{
					.type = CYMB_TOKEN_BAR_BAR,
					.info = {
						.position = {1, 25},
						.line = test3String,
						.hint = {test3String.string + 24, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 6},
					.info = {
						.position = {1, 28},
						.line = test3String,
						.hint = {test3String.string + 27, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CARET,
					.info = {
						.position = {1, 30},
						.line = test3String,
						.hint = {test3String.string + 29, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 7},
					.info = {
						.position = {1, 32},
						.line = test3String,
						.hint = {test3String.string + 31, 1}
					}
				},
				{
					.type = CYMB_TOKEN_LESS,
					.info = {
						.position = {1, 34},
						.line = test3String,
						.hint = {test3String.string + 33, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 8},
					.info = {
						.position = {1, 36},
						.line = test3String,
						.hint = {test3String.string + 35, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RIGHT_SHIFT,
					.info = {
						.position = {1, 38},
						.line = test3String,
						.hint = {test3String.string + 37, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 9},
					.info = {
						.position = {1, 41},
						.line = test3String,
						.hint = {test3String.string + 40, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SLASH,
					.info = {
						.position = {1, 43},
						.line = test3String,
						.hint = {test3String.string + 42, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 10},
					.info = {
						.position = {1, 45},
						.line = test3String,
						.hint = {test3String.string + 44, 2}
					}
				},
				{
					.type = CYMB_TOKEN_AMPERSAND_AMPERSAND,
					.info = {
						.position = {1, 48},
						.line = test3String,
						.hint = {test3String.string + 47, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 11},
					.info = {
						.position = {1, 51},
						.line = test3String,
						.hint = {test3String.string + 50, 2}
					}
				},
				{
					.type = CYMB_TOKEN_MINUS,
					.info = {
						.position = {1, 54},
						.line = test3String,
						.hint = {test3String.string + 53, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 12},
					.info = {
						.position = {1, 56},
						.line = test3String,
						.hint = {test3String.string + 55, 2}
					}
				},
				{
					.type = CYMB_TOKEN_GREATER_EQUAL,
					.info = {
						.position = {1, 59},
						.line = test3String,
						.hint = {test3String.string + 58, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 13},
					.info = {
						.position = {1, 62},
						.line = test3String,
						.hint = {test3String.string + 61, 2}
					}
				},
				{
					.type = CYMB_TOKEN_NOT_EQUAL,
					.info = {
						.position = {1, 65},
						.line = test3String,
						.hint = {test3String.string + 64, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 14},
					.info = {
						.position = {1, 68},
						.line = test3String,
						.hint = {test3String.string + 67, 2}
					}
				},
				{
					.type = CYMB_TOKEN_LESS_EQUAL,
					.info = {
						.position = {1, 71},
						.line = test3String,
						.hint = {test3String.string + 70, 2}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 15},
					.info = {
						.position = {1, 74},
						.line = test3String,
						.hint = {test3String.string + 73, 2}
					}
				},
				{
					.type = CYMB_TOKEN_BAR,
					.info = {
						.position = {1, 77},
						.line = test3String,
						.hint = {test3String.string + 76, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 16},
					.info = {
						.position = {1, 79},
						.line = test3String,
						.hint = {test3String.string + 78, 1}
					}
				},
				{
					.type = CYMB_TOKEN_GREATER,
					.info = {
						.position = {1, 82},
						.line = test3String,
						.hint = {test3String.string + 81, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 17},
					.info = {
						.position = {1, 84},
						.line = test3String,
						.hint = {test3String.string + 83, 2}
					}
				},
				{
					.type = CYMB_TOKEN_PERCENT,
					.info = {
						.position = {1, 87},
						.line = test3String,
						.hint = {test3String.string + 86, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 18},
					.info = {
						.position = {1, 89},
						.line = test3String,
						.hint = {test3String.string + 88, 2}
					}
				}
			},
			.count = 37
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 0},
						.info = tests[3].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[3].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 2},
						.info = tests[3].tokens.tokens[4].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_LS},
						.info = tests[3].tokens.tokens[3].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 3},
						.info = tests[3].tokens.tokens[6].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 4},
						.info = tests[3].tokens.tokens[8].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 5},
						.info = tests[3].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_MUL},
						.info = tests[3].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_SUM},
						.info = tests[3].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_EQ},
						.info = tests[3].tokens.tokens[5].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_AND},
						.info = tests[3].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 6},
						.info = tests[3].tokens.tokens[12].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 7},
						.info = tests[3].tokens.tokens[14].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 8},
						.info = tests[3].tokens.tokens[16].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 9},
						.info = tests[3].tokens.tokens[18].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 10},
						.info = tests[3].tokens.tokens[20].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_DIV},
						.info = tests[3].tokens.tokens[19].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_RS},
						.info = tests[3].tokens.tokens[17].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_LE},
						.info = tests[3].tokens.tokens[15].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_XOR},
						.info = tests[3].tokens.tokens[13].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 11},
						.info = tests[3].tokens.tokens[22].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 12},
						.info = tests[3].tokens.tokens[24].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_DIF},
						.info = tests[3].tokens.tokens[23].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 13},
						.info = tests[3].tokens.tokens[26].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_GEQ},
						.info = tests[3].tokens.tokens[25].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 14},
						.info = tests[3].tokens.tokens[28].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 15},
						.info = tests[3].tokens.tokens[30].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_LEQ},
						.info = tests[3].tokens.tokens[29].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_NEQ},
						.info = tests[3].tokens.tokens[27].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 16},
						.info = tests[3].tokens.tokens[32].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 17},
						.info = tests[3].tokens.tokens[34].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 18},
						.info = tests[3].tokens.tokens[36].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_MOD},
						.info = tests[3].tokens.tokens[35].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_GE},
						.info = tests[3].tokens.tokens[33].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_OR},
						.info = tests[3].tokens.tokens[31].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_LAND},
						.info = tests[3].tokens.tokens[21].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_LOR},
						.info = tests[3].tokens.tokens[11].info
					}
				},
				.count = 37
			}
	}, {}, 0}};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	tests[2].solution.tree->nodes[4].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[2];
	tests[2].solution.tree->nodes[4].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[3];
	tests[2].solution.tree->nodes[5].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[1];
	tests[2].solution.tree->nodes[5].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[4];
	tests[2].solution.tree->nodes[7].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[5];
	tests[2].solution.tree->nodes[7].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[6];
	tests[2].solution.tree->nodes[8].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[0];
	tests[2].solution.tree->nodes[8].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[7];
	tests[2].solution.tree->nodes[10].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[8];
	tests[2].solution.tree->nodes[10].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[9];

	tests[3].solution.tree->nodes[3].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[1];
	tests[3].solution.tree->nodes[3].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[2];
	tests[3].solution.tree->nodes[7].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[5];
	tests[3].solution.tree->nodes[7].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[6];
	tests[3].solution.tree->nodes[8].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[4];
	tests[3].solution.tree->nodes[8].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[7];
	tests[3].solution.tree->nodes[9].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[3];
	tests[3].solution.tree->nodes[9].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[8];
	tests[3].solution.tree->nodes[10].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[0];
	tests[3].solution.tree->nodes[10].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[9];
	tests[3].solution.tree->nodes[16].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[14];
	tests[3].solution.tree->nodes[16].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[15];
	tests[3].solution.tree->nodes[17].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[13];
	tests[3].solution.tree->nodes[17].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[16];
	tests[3].solution.tree->nodes[18].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[12];
	tests[3].solution.tree->nodes[18].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[17];
	tests[3].solution.tree->nodes[19].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[11];
	tests[3].solution.tree->nodes[19].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[18];
	tests[3].solution.tree->nodes[22].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[20];
	tests[3].solution.tree->nodes[22].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[21];
	tests[3].solution.tree->nodes[24].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[22];
	tests[3].solution.tree->nodes[24].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[23];
	tests[3].solution.tree->nodes[27].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[25];
	tests[3].solution.tree->nodes[27].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[26];
	tests[3].solution.tree->nodes[28].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[24];
	tests[3].solution.tree->nodes[28].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[27];
	tests[3].solution.tree->nodes[32].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[30];
	tests[3].solution.tree->nodes[32].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[31];
	tests[3].solution.tree->nodes[33].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[29];
	tests[3].solution.tree->nodes[33].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[32];
	tests[3].solution.tree->nodes[34].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[28];
	tests[3].solution.tree->nodes[34].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[33];
	tests[3].solution.tree->nodes[35].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[19];
	tests[3].solution.tree->nodes[35].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[34];
	tests[3].solution.tree->nodes[36].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[10];
	tests[3].solution.tree->nodes[36].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[35];

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseExpression, context);
	}

	--context->stringCount;
}

static void cymbTestTypes(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbConstString test0String = CYMB_STRING("int");
	const CymbConstString test1String = CYMB_STRING("float const*");
	const CymbConstString test2String = CYMB_STRING("const my_type* restrict* const");
	const CymbConstString test3String = CYMB_STRING("const const int const const");

	const CymbTreeTest tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = test0String,
						.hint = test0String
					}
				}
			},
			.count = 1
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[0].tokens.tokens[0].info
					}
				},
				.count = 1
			}
		}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_FLOAT,
					.info = {
						.position = {1, 1},
						.line = test1String,
						.hint = {test1String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 7},
						.line = test1String,
						.hint = {test1String.string + 6, 5}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 12},
						.line = test1String,
						.hint = {test1String.string + 11, 1}
					}
				}
			},
			.count = 3
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_FLOAT, .isConst = true},
						.info = tests[1].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_POINTER,
						.pointerNode = {},
						.info = tests[1].tokens.tokens[2].info
					}
				},
				.count = 2
			}
		}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 1},
						.line = test2String,
						.hint = {test2String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 7},
						.line = test2String,
						.hint = {test2String.string + 6, 7}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 14},
						.line = test2String,
						.hint = {test2String.string + 13, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RESTRICT,
					.info = {
						.position = {1, 16},
						.line = test2String,
						.hint = {test2String.string + 15, 8}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 24},
						.line = test2String,
						.hint = {test2String.string + 23, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 26},
						.line = test2String,
						.hint = {test2String.string + 25, 5}
					}
				}
			},
			.count = 6
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_OTHER, .isConst = true},
						.info = tests[2].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_POINTER,
						.pointerNode = {.isRestrict = true},
						.info = tests[2].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_POINTER,
						.pointerNode = {.isConst = true},
						.info = tests[2].tokens.tokens[4].info
					}
				},
				.count = 3
			}
		}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 1},
						.line = test3String,
						.hint = {test3String.string + 0, 5}
					}
				},
								{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 7},
						.line = test3String,
						.hint = {test3String.string + 6, 5}
					}
				},
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 13},
						.line = test3String,
						.hint = {test3String.string + 12, 3}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 17},
						.line = test3String,
						.hint = {test3String.string + 16, 5}
					}
				},
								{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 23},
						.line = test3String,
						.hint = {test3String.string + 22, 5}
					}
				},
			},
			.count = 5
		}, CYMB_PARSE_INVALID, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT, .isConst = true},
						.info = tests[3].tokens.tokens[2].info
					}
				},
				.count = 1
			}
		}, {
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_MULTIPLE_CONST,
					.info = tests[3].tokens.tokens[1].info
				},
				{
					.type = CYMB_MULTIPLE_CONST,
					.info = tests[3].tokens.tokens[4].info
				},
				{
					.type = CYMB_MULTIPLE_CONST,
					.info = tests[3].tokens.tokens[3].info
				}
			},
			.count = 3
		}, 0}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	tests[1].solution.tree->nodes[1].pointerNode.pointedNode = &tests[1].solution.tree->nodes[0];

	tests[2].solution.tree->nodes[1].pointerNode.pointedNode = &tests[2].solution.tree->nodes[0];
	tests[2].solution.tree->nodes[2].pointerNode.pointedNode = &tests[2].solution.tree->nodes[1];

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseType, context);
	}

	--context->stringCount;
}

static void cymbTestStatements(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbConstString test2String = CYMB_STRING("return;");
	const CymbConstString test3String = CYMB_STRING("return 1 + 2; 3");
	const CymbConstString test4String = CYMB_STRING("int my_var;");
	const CymbConstString test5String = CYMB_STRING("const long other_var = 1;");
	const CymbConstString test6String = CYMB_STRING("while(a > 5){int b = 3; return a + b;}");
	const CymbConstString test7String = CYMB_STRING("while(0)return;return;");

	const CymbTreeTest tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_RETURN}
			},
			.count = 1
		}, CYMB_PARSE_NO_MATCH, {}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{.type = CYMB_TOKEN_SEMICOLON}
			},
			.count = 1
		}, CYMB_PARSE_NO_MATCH, {}, {}, 0},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 1},
						.line = test2String,
						.hint = {test2String.string + 0, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 7},
						.line = test2String,
						.hint = {test2String.string + 6, 1}
					}
				}
			},
			.count = 2
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_RETURN,
						.returnNode = nullptr,
						.info = tests[2].tokens.tokens[0].info
					}
				},
				.count = 1
			}
		}, {}, 2},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 1},
						.line = test3String,
						.hint = {test3String.string + 0, 6}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 8},
						.line = test3String,
						.hint = {test3String.string + 7, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 10},
						.line = test3String,
						.hint = {test3String.string + 9, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 2},
					.info = {
						.position = {1, 12},
						.line = test3String,
						.hint = {test3String.string + 11, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 13},
						.line = test3String,
						.hint = {test3String.string + 12, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 3},
					.info = {
						.position = {1, 15},
						.line = test3String,
						.hint = {test3String.string + 14, 1}
					}
				}
			},
			.count = 6
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[3].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 2},
						.info = tests[3].tokens.tokens[3].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {CYMB_BINARY_OPERATOR_SUM},
						.info = tests[3].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[3].tokens.tokens[0].info
					}
				},
				.count = 4
			}
		}, {}, 5},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = test4String,
						.hint = {test4String.string + 0, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = test4String,
						.hint = {test4String.string + 4, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 11},
						.line = test4String,
						.hint = {test4String.string + 10, 1}
					}
				}
			},
			.count = 3,
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[4].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[4].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_DECLARATION,
						.info = tests[4].tokens.tokens[1].info
					}
				},
				.count = 3
			}
		}, {}, 3},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 1},
						.line = test5String,
						.hint = {test5String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_LONG,
					.info = {
						.position = {1, 7},
						.line = test5String,
						.hint = {test5String.string + 6, 4}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 12},
						.line = test5String,
						.hint = {test5String.string + 11, 9}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL,
					.info = {
						.position = {1, 22},
						.line = test5String,
						.hint = {test5String.string + 21, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 24},
						.line = test5String,
						.hint = {test5String.string + 23, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 25},
						.line = test5String,
						.hint = {test5String.string + 24, 1}
					}
				}
			},
			.count = 6
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_LONG, .isConst = true},
						.info = tests[5].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[5].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[5].tokens.tokens[4].info
					},
					{
						.type = CYMB_NODE_DECLARATION,
						.info = tests[5].tokens.tokens[2].info
					}
				},
				.count = 4
			}
		}, {}, 6},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_WHILE,
					.info = {
						.position = {1, 1},
						.line = test6String,
						.hint = {test6String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 6},
						.line = test6String,
						.hint = {test6String.string + 5, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 7},
						.line = test6String,
						.hint = {test6String.string + 6, 1}
					}
				},
				{
					.type = CYMB_TOKEN_GREATER,
					.info = {
						.position = {1, 9},
						.line = test6String,
						.hint = {test6String.string + 8, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 11},
						.line = test6String,
						.hint = {test6String.string + 10, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 12},
						.line = test6String,
						.hint = {test6String.string + 11, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 13},
						.line = test6String,
						.hint = {test6String.string + 12, 1}
					}
				},
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 14},
						.line = test6String,
						.hint = {test6String.string + 13, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 18},
						.line = test6String,
						.hint = {test6String.string + 17, 1}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL,
					.info = {
						.position = {1, 20},
						.line = test6String,
						.hint = {test6String.string + 19, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 3},
					.info = {
						.position = {1, 22},
						.line = test6String,
						.hint = {test6String.string + 21, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 23},
						.line = test6String,
						.hint = {test6String.string + 22, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 25},
						.line = test6String,
						.hint = {test6String.string + 24, 6}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 32},
						.line = test6String,
						.hint = {test6String.string + 31, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 34},
						.line = test6String,
						.hint = {test6String.string + 33, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 36},
						.line = test6String,
						.hint = {test6String.string + 35, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 37},
						.line = test6String,
						.hint = {test6String.string + 36, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 38},
						.line = test6String,
						.hint = {test6String.string + 37, 1}
					}
				}
			},
			.count = 18
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[6].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 5},
						.info = tests[6].tokens.tokens[4].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {.operator = CYMB_BINARY_OPERATOR_GE},
						.info = tests[6].tokens.tokens[3].info
					},
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[6].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[6].tokens.tokens[8].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 3},
						.info = tests[6].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_DECLARATION,
						.info = tests[6].tokens.tokens[8].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[6].tokens.tokens[13].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[6].tokens.tokens[15].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {.operator = CYMB_BINARY_OPERATOR_SUM},
						.info = tests[6].tokens.tokens[14].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[6].tokens.tokens[12].info
					},
					{
						.type = CYMB_NODE_WHILE,
						.whileNode = {.body = {.count = 2}},
						.info = tests[6].tokens.tokens[0].info
					}
				},
				.count = 12,
				.children = (CymbNode*[2]){}
			},
			.childCount = 2
		}, {}, 18},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_WHILE,
					.info = {
						.position = {1, 1},
						.line = test7String,
						.hint = {test7String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 6},
						.line = test7String,
						.hint = {test7String.string + 5, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 0},
					.info = {
						.position = {1, 7},
						.line = test7String,
						.hint = {test7String.string + 6, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 8},
						.line = test7String,
						.hint = {test7String.string + 7, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 9},
						.line = test7String,
						.hint = {test7String.string + 8, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 15},
						.line = test7String,
						.hint = {test7String.string + 14, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 16},
						.line = test7String,
						.hint = {test7String.string + 15, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 22},
						.line = test7String,
						.hint = {test7String.string + 21, 1}
					}
				},
			},
			.count = 8
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 0},
						.info = tests[7].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.returnNode = nullptr,
						.info = tests[7].tokens.tokens[4].info
					},
					{
						.type = CYMB_NODE_WHILE,
						.whileNode = {.body = {.count = 1}},
						.info = tests[7].tokens.tokens[0].info
					}
				},
				.count = 3,
				.children = (CymbNode*[1]){}
			},
			.childCount = 1
		}, {}, 6}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	tests[3].solution.tree->nodes[2].binaryOperatorNode.leftNode = &tests[3].solution.tree->nodes[0];
	tests[3].solution.tree->nodes[2].binaryOperatorNode.rightNode = &tests[3].solution.tree->nodes[1];
	tests[3].solution.tree->nodes[3].returnNode = &tests[3].solution.tree->nodes[2];

	tests[4].solution.tree->nodes[2].declarationNode.type = &tests[4].solution.tree->nodes[0];
	tests[4].solution.tree->nodes[2].declarationNode.identifier = &tests[4].solution.tree->nodes[1];

	tests[5].solution.tree->nodes[3].declarationNode.type = &tests[5].solution.tree->nodes[0];
	tests[5].solution.tree->nodes[3].declarationNode.identifier = &tests[5].solution.tree->nodes[1];
	tests[5].solution.tree->nodes[3].declarationNode.initializer = &tests[5].solution.tree->nodes[2];

	tests[6].solution.tree->nodes[2].binaryOperatorNode.leftNode = &tests[6].solution.tree->nodes[0];
	tests[6].solution.tree->nodes[2].binaryOperatorNode.rightNode = &tests[6].solution.tree->nodes[1];
	tests[6].solution.tree->nodes[6].declarationNode.type = &tests[6].solution.tree->nodes[3];
	tests[6].solution.tree->nodes[6].declarationNode.identifier = &tests[6].solution.tree->nodes[4];
	tests[6].solution.tree->nodes[6].declarationNode.initializer = &tests[6].solution.tree->nodes[5];
	tests[6].solution.tree->nodes[9].binaryOperatorNode.leftNode = &tests[6].solution.tree->nodes[7];
	tests[6].solution.tree->nodes[9].binaryOperatorNode.rightNode = &tests[6].solution.tree->nodes[8];
	tests[6].solution.tree->nodes[10].returnNode = &tests[6].solution.tree->nodes[9];
	tests[6].solution.tree->nodes[11].whileNode.expression = &tests[6].solution.tree->nodes[2];
	tests[6].solution.tree->nodes[11].whileNode.body.nodes = tests[6].solution.tree->children;
	tests[6].solution.tree->children[0] = &tests[6].solution.tree->nodes[6];
	tests[6].solution.tree->children[1] = &tests[6].solution.tree->nodes[10];

	tests[7].solution.tree->nodes[2].whileNode.expression = &tests[7].solution.tree->nodes[0];
	tests[7].solution.tree->nodes[2].whileNode.body.nodes = tests[7].solution.tree->children;
	tests[7].solution.tree->children[0] = &tests[7].solution.tree->nodes[1];

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseStatement, context);
	}

	--context->stringCount;
}

static void cymbTestFunctions(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbConstString test0String = CYMB_STRING("int some_func(void){return 1 + 2; return; return 0;}");
	const CymbConstString test1String = CYMB_STRING("const float* some_other_func(){}");
	const CymbConstString test2String = CYMB_STRING("int func(){const int a = 1; return a + 2;};");
	const CymbConstString test3String = CYMB_STRING("void add(const int* const a, float b){return;}");

	const CymbTreeTest tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = test0String,
						.hint = {test0String.string + 0, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = test0String,
						.hint = {test0String.string + 4, 9}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 14},
						.line = test0String,
						.hint = {test0String.string + 13, 1}
					}
				},
				{
					.type = CYMB_TOKEN_VOID,
					.info = {
						.position = {1, 15},
						.line = test0String,
						.hint = {test0String.string + 14, 4}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 19},
						.line = test0String,
						.hint = {test0String.string + 18, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 20},
						.line = test0String,
						.hint = {test0String.string + 19, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 21},
						.line = test0String,
						.hint = {test0String.string + 20, 6}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 28},
						.line = test0String,
						.hint = {test0String.string + 27, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 30},
						.line = test0String,
						.hint = {test0String.string + 29, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 2},
					.info = {
						.position = {1, 32},
						.line = test0String,
						.hint = {test0String.string + 31, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 33},
						.line = test0String,
						.hint = {test0String.string + 32, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 35},
						.line = test0String,
						.hint = {test0String.string + 34, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 41},
						.line = test0String,
						.hint = {test0String.string + 40, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 43},
						.line = test0String,
						.hint = {test0String.string + 42, 6}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 0},
					.info = {
						.position = {1, 50},
						.line = test0String,
						.hint = {test0String.string + 49, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 51},
						.line = test0String,
						.hint = {test0String.string + 50, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 52},
						.line = test0String,
						.hint = {test0String.string + 51, 3}
					}
				}
			},
			.count = 17
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[0].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {},
						.info = tests[0].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[0].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[0].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 2},
						.info = tests[0].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {.operator = CYMB_BINARY_OPERATOR_SUM},
						.info = tests[0].tokens.tokens[8].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[0].tokens.tokens[6].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.returnNode = nullptr,
						.info = tests[0].tokens.tokens[11].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 0},
						.info = tests[0].tokens.tokens[14].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[0].tokens.tokens[13].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {.statements = {.count = 3}},
						.info = tests[0].tokens.tokens[1].info
					}
				},
				.count = 11,
				.children = (CymbNode*[3]){}
			},
			.childCount = 3
		}, {}, 17},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 1},
						.line = test1String,
						.hint = {test1String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_FLOAT,
					.info = {
						.position = {1, 7},
						.line = test1String,
						.hint = {test1String.string + 6, 5}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 12},
						.line = test1String,
						.hint = {test1String.string + 11, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 14},
						.line = test1String,
						.hint = {test1String.string + 13, 15}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 29},
						.line = test1String,
						.hint = {test1String.string + 28, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 30},
						.line = test1String,
						.hint = {test1String.string + 29, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 31},
						.line = test1String,
						.hint = {test1String.string + 30, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 32},
						.line = test1String,
						.hint = {test1String.string + 31, 1}
					}
				}
			},
			.count = 8
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_FLOAT, .isConst = true},
						.info = tests[1].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_POINTER,
						.pointerNode = {},
						.info = tests[1].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {},
						.info = tests[1].tokens.tokens[2].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[1].tokens.tokens[3].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {},
						.info = tests[1].tokens.tokens[3].info
					}
				},
				.count = 5
			}
		}, {}, 8},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = test2String,
						.hint = {test2String.string + 0, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = test2String,
						.hint = {test2String.string + 4, 4}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 9},
						.line = test2String,
						.hint = {test2String.string + 8, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 10},
						.line = test2String,
						.hint = {test2String.string + 9, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 11},
						.line = test2String,
						.hint = {test2String.string + 10, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 12},
						.line = test2String,
						.hint = {test2String.string + 11, 5}
					}
				},
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 18},
						.line = test2String,
						.hint = {test2String.string + 17, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 22},
						.line = test2String,
						.hint = {test2String.string + 21, 1}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL,
					.info = {
						.position = {1, 24},
						.line = test2String,
						.hint = {test2String.string + 23, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 26},
						.line = test2String,
						.hint = {test2String.string + 25, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 27},
						.line = test2String,
						.hint = {test2String.string + 26, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 29},
						.line = test2String,
						.hint = {test2String.string + 28, 6}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 36},
						.line = test2String,
						.hint = {test2String.string + 35, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 38},
						.line = test2String,
						.hint = {test2String.string + 37, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 2},
					.info = {
						.position = {1, 40},
						.line = test2String,
						.hint = {test2String.string + 39, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 41},
						.line = test2String,
						.hint = {test2String.string + 40, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 42},
						.line = test2String,
						.hint = {test2String.string + 41, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 41},
						.line = test2String,
						.hint = {test2String.string + 42, 1}
					}
				}
			},
			.count = 18
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[2].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {},
						.info = tests[2].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[2].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT, .isConst = true},
						.info = tests[2].tokens.tokens[6].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[2].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[2].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_DECLARATION,
						.info = tests[2].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[2].tokens.tokens[12].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 2},
						.info = tests[2].tokens.tokens[14].info
					},
					{
						.type = CYMB_NODE_BINARY_OPERATOR,
						.binaryOperatorNode = {.operator = CYMB_BINARY_OPERATOR_SUM},
						.info = tests[2].tokens.tokens[13].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[2].tokens.tokens[11].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {.statements = {.count = 2}},
						.info = tests[2].tokens.tokens[1].info
					}
				},
				.count = 12,
				.children = (CymbNode*[2]){}
			},
			.childCount = 2
		}, {}, 17},
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_VOID,
					.info = {
						.position = {1, 1},
						.line = test3String,
						.hint = {test3String.string + 0, 4}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 6},
						.line = test3String,
						.hint = {test3String.string + 5, 3}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 11},
						.line = test3String,
						.hint = {test3String.string + 10, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 12},
						.line = test3String,
						.hint = {test3String.string + 11, 5}
					}
				},
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 18},
						.line = test3String,
						.hint = {test3String.string + 17, 3}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 19},
						.line = test3String,
						.hint = {test3String.string + 18, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 21},
						.line = test3String,
						.hint = {test3String.string + 20, 5}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 27},
						.line = test3String,
						.hint = {test3String.string + 26, 1}
					}
				},
				{
					.type = CYMB_TOKEN_COMMA,
					.info = {
						.position = {1, 28},
						.line = test3String,
						.hint = {test3String.string + 27, 1}
					}
				},
				{
					.type = CYMB_TOKEN_FLOAT,
					.info = {
						.position = {1, 30},
						.line = test3String,
						.hint = {test3String.string + 29, 5}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 36},
						.line = test3String,
						.hint = {test3String.string + 35, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 37},
						.line = test3String,
						.hint = {test3String.string + 36, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 38},
						.line = test3String,
						.hint = {test3String.string + 37, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 39},
						.line = test3String,
						.hint = {test3String.string + 38, 6}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 45},
						.line = test3String,
						.hint = {test3String.string + 44, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 46},
						.line = test3String,
						.hint = {test3String.string + 45, 1}
					}
				}
			},
			.count = 16
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_VOID},
						.info = tests[3].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT, .isConst = true},
						.info = tests[3].tokens.tokens[4].info
					},
					{
						.type = CYMB_NODE_POINTER,
						.pointerNode = {.isConst = true},
						.info = tests[3].tokens.tokens[5].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[3].tokens.tokens[7].info
					},
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_FLOAT},
						.info = tests[3].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[3].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {.parameterTypes = {.count = 2}},
						.info = tests[3].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[3].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.returnNode = nullptr,
						.info = tests[3].tokens.tokens[13].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {.parameters = {.count = 2}, .statements = {.count = 1}},
						.info = tests[3].tokens.tokens[1].info
					},
				},
				.count = 10,
				.children = (CymbNode*[5]){}
			},
			.childCount = 5
		}, {}, 16}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	tests[0].solution.tree->nodes[1].functionTypeNode.returnType = &tests[0].solution.tree->nodes[0];
	tests[0].solution.tree->nodes[5].binaryOperatorNode.leftNode = &tests[0].solution.tree->nodes[3];
	tests[0].solution.tree->nodes[5].binaryOperatorNode.rightNode = &tests[0].solution.tree->nodes[4];
	tests[0].solution.tree->nodes[6].returnNode = &tests[0].solution.tree->nodes[5];
	tests[0].solution.tree->nodes[9].returnNode = &tests[0].solution.tree->nodes[8];
	tests[0].solution.tree->nodes[10].functionNode.name = &tests[0].solution.tree->nodes[2];
	tests[0].solution.tree->nodes[10].functionNode.type = &tests[0].solution.tree->nodes[1];
	tests[0].solution.tree->nodes[10].functionNode.statements.nodes = &tests[0].solution.tree->children[0];
	tests[0].solution.tree->children[0] = tests[0].solution.tree->nodes + 6;
	tests[0].solution.tree->children[1] = tests[0].solution.tree->nodes + 7;
	tests[0].solution.tree->children[2] = tests[0].solution.tree->nodes + 9;

	tests[1].solution.tree->nodes[1].pointerNode.pointedNode = &tests[1].solution.tree->nodes[0];
	tests[1].solution.tree->nodes[2].functionTypeNode.returnType = &tests[1].solution.tree->nodes[1];
	tests[1].solution.tree->nodes[4].functionNode.type = &tests[1].solution.tree->nodes[2];
	tests[1].solution.tree->nodes[4].functionNode.name = &tests[1].solution.tree->nodes[3];

	tests[2].solution.tree->nodes[1].functionTypeNode.returnType = &tests[2].solution.tree->nodes[0];
	tests[2].solution.tree->nodes[6].declarationNode.type = &tests[2].solution.tree->nodes[3];
	tests[2].solution.tree->nodes[6].declarationNode.identifier = &tests[2].solution.tree->nodes[4];
	tests[2].solution.tree->nodes[6].declarationNode.initializer = &tests[2].solution.tree->nodes[5];
	tests[2].solution.tree->nodes[9].binaryOperatorNode.leftNode = &tests[2].solution.tree->nodes[7];
	tests[2].solution.tree->nodes[9].binaryOperatorNode.rightNode = &tests[2].solution.tree->nodes[8];
	tests[2].solution.tree->nodes[10].returnNode = &tests[2].solution.tree->nodes[9];
	tests[2].solution.tree->nodes[11].functionNode.type = &tests[2].solution.tree->nodes[1];
	tests[2].solution.tree->nodes[11].functionNode.name = &tests[2].solution.tree->nodes[2];
	tests[2].solution.tree->nodes[11].functionNode.statements.nodes = &tests[2].solution.tree->children[0];
	tests[2].solution.tree->children[0] = tests[2].solution.tree->nodes + 6;
	tests[2].solution.tree->children[1] = tests[2].solution.tree->nodes + 10;

	tests[3].solution.tree->nodes[2].pointerNode.pointedNode = &tests[3].solution.tree->nodes[1];
	tests[3].solution.tree->nodes[6].functionTypeNode.returnType = &tests[3].solution.tree->nodes[0];
	tests[3].solution.tree->nodes[6].functionTypeNode.parameterTypes.nodes = &tests[3].solution.tree->children[0];
	tests[3].solution.tree->nodes[9].functionNode.type = &tests[3].solution.tree->nodes[6];
	tests[3].solution.tree->nodes[9].functionNode.name = &tests[3].solution.tree->nodes[7];
	tests[3].solution.tree->nodes[9].functionNode.parameters.nodes = &tests[3].solution.tree->children[2];
	tests[3].solution.tree->nodes[9].functionNode.statements.nodes = &tests[3].solution.tree->children[4];
	tests[3].solution.tree->children[0] = tests[3].solution.tree->nodes + 2;
	tests[3].solution.tree->children[1] = tests[3].solution.tree->nodes + 4;
	tests[3].solution.tree->children[2] = tests[3].solution.tree->nodes + 3;
	tests[3].solution.tree->children[3] = tests[3].solution.tree->nodes + 5;
	tests[3].solution.tree->children[4] = tests[3].solution.tree->nodes + 8;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseFunction, context);
	}

	--context->stringCount;
}

static void cymbTestProgram(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbConstString test0String = CYMB_STRING("int some_func(){return 1;} int main(void){return 0;}");

	const CymbTreeTest tests[] = {
		{{
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = test0String,
						.hint = {test0String.string + 0, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = test0String,
						.hint = {test0String.string + 4, 9}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 14},
						.line = test0String,
						.hint = {test0String.string + 13, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 15},
						.line = test0String,
						.hint = {test0String.string + 14, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 16},
						.line = test0String,
						.hint = {test0String.string + 15, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 17},
						.line = test0String,
						.hint = {test0String.string + 16, 6}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 24},
						.line = test0String,
						.hint = {test0String.string + 23, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 25},
						.line = test0String,
						.hint = {test0String.string + 24, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 26},
						.line = test0String,
						.hint = {test0String.string + 25, 1}
					}
				},
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 28},
						.line = test0String,
						.hint = {test0String.string + 27, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 32},
						.line = test0String,
						.hint = {test0String.string + 31, 4}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 36},
						.line = test0String,
						.hint = {test0String.string + 35, 1}
					}
				},
				{
					.type = CYMB_TOKEN_VOID,
					.info = {
						.position = {1, 37},
						.line = test0String,
						.hint = {test0String.string + 36, 4}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 41},
						.line = test0String,
						.hint = {test0String.string + 40, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACE,
					.info = {
						.position = {1, 42},
						.line = test0String,
						.hint = {test0String.string + 41, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 43},
						.line = test0String,
						.hint = {test0String.string + 42, 6}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 0},
					.info = {
						.position = {1, 50},
						.line = test0String,
						.hint = {test0String.string + 49, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 51},
						.line = test0String,
						.hint = {test0String.string + 50, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 52},
						.line = test0String,
						.hint = {test0String.string + 51, 1}
					}
				}
			},
			.count = 19
		}, CYMB_PARSE_MATCH, {
			.tree = &(CymbTree){
				.nodes = (CymbNode[]){
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[0].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {},
						.info = tests[0].tokens.tokens[0].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[0].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 1},
						.info = tests[0].tokens.tokens[6].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[0].tokens.tokens[5].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {.statements = {.count = 1}},
						.info = tests[0].tokens.tokens[1].info
					},
					{
						.type = CYMB_NODE_TYPE,
						.typeNode = {.type = CYMB_TYPE_INT},
						.info = tests[0].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_FUNCTION_TYPE,
						.functionTypeNode = {},
						.info = tests[0].tokens.tokens[9].info
					},
					{
						.type = CYMB_NODE_IDENTIFIER,
						.info = tests[0].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_CONSTANT,
						.constantNode = {CYMB_CONSTANT_INT, 0},
						.info = tests[0].tokens.tokens[16].info
					},
					{
						.type = CYMB_NODE_RETURN,
						.info = tests[0].tokens.tokens[15].info
					},
					{
						.type = CYMB_NODE_FUNCTION,
						.functionNode = {.statements = {.count = 1}},
						.info = tests[0].tokens.tokens[10].info
					},
					{
						.type = CYMB_NODE_PROGRAM,
						.programNode = {.children = {.count = 2}}
					}
				},
				.count = 13,
				.children = (CymbNode*[4]){}
			},
			.childCount = 4
		}, {}, 19}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	tests[0].solution.tree->nodes[1].functionTypeNode.returnType = &tests[0].solution.tree->nodes[0];
	tests[0].solution.tree->nodes[4].returnNode = &tests[0].solution.tree->nodes[3];
	tests[0].solution.tree->nodes[5].functionNode.type = &tests[0].solution.tree->nodes[1];
	tests[0].solution.tree->nodes[5].functionNode.name = &tests[0].solution.tree->nodes[2];
	tests[0].solution.tree->nodes[5].functionNode.statements.nodes = &tests[0].solution.tree->children[0];
	tests[0].solution.tree->nodes[7].functionTypeNode.returnType = &tests[0].solution.tree->nodes[6];
	tests[0].solution.tree->nodes[10].returnNode = &tests[0].solution.tree->nodes[9];
	tests[0].solution.tree->nodes[11].functionNode.type = &tests[0].solution.tree->nodes[7];
	tests[0].solution.tree->nodes[11].functionNode.name = &tests[0].solution.tree->nodes[8];
	tests[0].solution.tree->nodes[11].functionNode.statements.nodes = &tests[0].solution.tree->children[1];
	tests[0].solution.tree->nodes[12].programNode.children.nodes = &tests[0].solution.tree->children[2];
	tests[0].solution.tree->children[0] = tests[0].solution.tree->nodes + 4;
	tests[0].solution.tree->children[1] = tests[0].solution.tree->nodes + 10;
	tests[0].solution.tree->children[2] = tests[0].solution.tree->nodes + 5;
	tests[0].solution.tree->children[3] = tests[0].solution.tree->nodes + 11;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseProgram, context);
	}

	--context->stringCount;
}

void cymbTestTrees(CymbTestContext* const context)
{
	cymbTestParentheses(context);
	cymbTestExpressions(context);
	cymbTestTypes(context);
	cymbTestStatements(context);
	cymbTestFunctions(context);
	cymbTestProgram(context);
}
