#include "test.h"

#include <stdio.h>

#include "cymb/tree.h"

typedef struct CymbTreeTest
{
	CymbTokenList tokens;
	CymbResult result;
	CymbTree solution;
	CymbDiagnosticList diagnostics;
	size_t endTokensOffset;
} CymbTreeTest;

static void cymbCompareNodes(const CymbNode* const first, const CymbNode* const second, CymbTestContext* const context, size_t* const nodeCount)
{
	++*nodeCount;
	cymbContextSetIndex(context, *nodeCount);

	if(!first && !second)
	{
		return;
	}

	if(!first || !second)
	{
		cymbFail(context, "Node is null.");
		return;
	}


	if(first->type != second->type)
	{
		cymbFail(context, "Wrong node type.");
		return;
	}

	cymbCompareDiagnosticInfo(&first->info, &second->info, context);

	switch(first->type)
	{
		case CYMB_NODE_CONSTANT:
		{
			if(first->constantNode.type != second->constantNode.type || first->constantNode.value != second->constantNode.value)
			{
				cymbFail(context, "Wrong constant.");
			}
			break;
		}

		case CYMB_NODE_IDENTIFIER:
		{
			break;
		}

		case CYMB_NODE_UNARY_OPERATOR:
		{
			if(first->unaryOperatorNode.operator != second->unaryOperatorNode.operator)
			{
				cymbFail(context, "Wrong unary operator type.");
			}
			cymbCompareNodes(first->unaryOperatorNode.node, second->unaryOperatorNode.node, context, nodeCount);
			break;
		}

		case CYMB_NODE_BINARY_OPERATOR:
		{
			if(first->binaryOperatorNode.operator != second->binaryOperatorNode.operator)
			{
				cymbFail(context, "Wrong binary operator type.");
			}
			cymbCompareNodes(first->binaryOperatorNode.leftNode, second->binaryOperatorNode.leftNode, context, nodeCount);
			cymbCompareNodes(first->binaryOperatorNode.rightNode, second->binaryOperatorNode.rightNode, context, nodeCount);
			break;
		}

		case CYMB_NODE_WHILE:
		{
			cymbCompareNodes(first->whileNode.expression, second->whileNode.expression, context, nodeCount);

			const CymbNodeChild* firstChild = first->whileNode.body;
			const CymbNodeChild* secondChild = second->whileNode.body;

			while(firstChild)
			{
				if(!secondChild)
				{
					cymbFail(context, "Unexpected child node.");
					return;
				}

				cymbCompareNodes(firstChild->node, secondChild->node, context, nodeCount);

				firstChild = firstChild->next;
				secondChild = secondChild->next;
			}

			if(secondChild)
			{
				cymbFail(context, "Missing child node.");
			}

			break;
		}

		case CYMB_NODE_RETURN:
		{
			cymbCompareNodes(first->returnNode, second->returnNode, context, nodeCount);
			break;
		}

		case CYMB_NODE_TYPE:
		{
			if(first->typeNode.type != second->typeNode.type)
			{
				cymbFail(context, "Wrong type type.");
			}
			if(first->typeNode.isConst != second->typeNode.isConst)
			{
				cymbFail(context, "Wrong type const.");
			}
			if(first->typeNode.isStatic != second->typeNode.isStatic)
			{
				cymbFail(context, "Wrong type static.");
			}
			break;
		}

		case CYMB_NODE_POINTER:
		{
			cymbCompareNodes(first->pointerNode.pointedNode, second->pointerNode.pointedNode, context, nodeCount);
			if(first->pointerNode.isConst != second->pointerNode.isConst)
			{
				cymbFail(context, "Wrong pointer const.");
			}
			if(first->pointerNode.isRestrict != second->pointerNode.isRestrict)
			{
				cymbFail(context, "Wrong pointer restrict.");
			}
			break;
		}

		case CYMB_NODE_FUNCTION_TYPE:
		{
			cymbCompareNodes(first->functionTypeNode.returnType, second->functionTypeNode.returnType, context, nodeCount);

			const CymbNodeChild* firstChild = first->functionTypeNode.parameterTypes;
			const CymbNodeChild* secondChild = second->functionTypeNode.parameterTypes;

			while(firstChild)
			{
				if(!secondChild)
				{
					cymbFail(context, "Unexpected child node.");
					return;
				}

				cymbCompareNodes(firstChild->node, secondChild->node, context, nodeCount);

				firstChild = firstChild->next;
				secondChild = secondChild->next;
			}

			if(secondChild)
			{
				cymbFail(context, "Missing child node.");
			}

			break;
		}

		case CYMB_NODE_DECLARATION:
		{
			cymbCompareNodes(first->declarationNode.identifier, second->declarationNode.identifier, context, nodeCount);
			cymbCompareNodes(first->declarationNode.type, second->declarationNode.type, context, nodeCount);
			cymbCompareNodes(first->declarationNode.initializer, second->declarationNode.initializer, context, nodeCount);
			break;
		}

		case CYMB_NODE_FUNCTION:
		{
			cymbCompareNodes(first->functionNode.name, second->functionNode.name, context, nodeCount);
			cymbCompareNodes(first->functionNode.type, second->functionNode.type, context, nodeCount);

			const CymbNodeChild* firstChild = first->functionNode.parameters;
			const CymbNodeChild* secondChild = second->functionNode.parameters;

			while(firstChild)
			{
				if(!secondChild)
				{
					cymbFail(context, "Unexpected child node.");
					return;
				}

				cymbCompareNodes(firstChild->node, secondChild->node, context, nodeCount);

				firstChild = firstChild->next;
				secondChild = secondChild->next;
			}

			if(secondChild)
			{
				cymbFail(context, "Missing child node.");
			}

			firstChild = first->functionNode.statements;
			secondChild = second->functionNode.statements;

			while(firstChild)
			{
				if(!secondChild)
				{
					cymbFail(context, "Unexpected child node.");
					return;
				}

				cymbCompareNodes(firstChild->node, secondChild->node, context, nodeCount);

				firstChild = firstChild->next;
				secondChild = secondChild->next;
			}

			if(secondChild)
			{
				cymbFail(context, "Missing child node.");
			}

			break;
		}

		case CYMB_NODE_PROGRAM:
		{
			const CymbNodeChild* firstChild = first->programNode.children;
			const CymbNodeChild* secondChild = second->programNode.children;

			while(firstChild)
			{
				if(!secondChild)
				{
					cymbFail(context, "Unexpected child node.");
					return;
				}

				cymbCompareNodes(firstChild->node, secondChild->node, context, nodeCount);

				firstChild = firstChild->next;
				secondChild = secondChild->next;
			}

			if(secondChild)
			{
				cymbFail(context, "Missing child node.");
			}

			break;
		}

		default:
		{
			cymbFail(context, "Unknown node type.");
			break;
		}
	}
}

static void cymbCompareTrees(const CymbTree* const first, const CymbTree* const second, CymbTestContext* const context)
{
	cymbContextPush(context, "node");

	size_t nodeCount = 0;
	cymbCompareNodes(first->root, second->root, context, &nodeCount);

	cymbContextPop(context);
}

static void cymbDoTreeTest(const CymbTreeTest* const test, const CymbTreeFunction function, CymbTestContext* const context)
{
	const CymbArenaSave save = cymbArenaSave(&context->arena);

	CymbTree tree = {.arena = &context->arena};

	CymbTokenList tokens = test->tokens;

	const CymbResult result = function(&tree, &tokens, &context->diagnostics);
	if(result != test->result)
	{
		cymbFail(context, "Wrong result.");
		goto end;
	}

	cymbCompareDiagnostics(&context->diagnostics, &test->diagnostics, context);

	if(test->endTokensOffset > 0 && (tokens.tokens != test->tokens.tokens + test->endTokensOffset || tokens.count != test->tokens.count - test->endTokensOffset))
	{
		cymbFail(context, "Wrong end tokens.");
	}

	cymbCompareTrees(&tree, &test->solution, context);

	end:
	cymbArenaRestore(&context->arena, save);
	cymbDiagnosticListFree(&context->diagnostics);
}

static void cymbTestParentheses(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	struct
	{
		CymbTokenList tokens;
		CymbDirection direction;
		CymbResult result;
		size_t startIndex;
		size_t endIndex;
		CymbDiagnosticList diagnostics;
	} tests[] = {
		{{
			.tokens = (CymbToken[]){
				{.type = CYMB_TOKEN_PLUS}
			},
			.count = 1
		}, CYMB_DIRECTION_FORWARD, CYMB_NO_MATCH, 0, 0, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_FORWARD, CYMB_INVALID, 0, 0, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_BACKWARD, CYMB_INVALID, 0, 0, {}},
		{{
			.tokens = (CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 2
		}, CYMB_DIRECTION_FORWARD, CYMB_SUCCESS, 0, 1, {}},
		{{
			.tokens = (CymbToken[]){
				{.type = CYMB_TOKEN_OPEN_PARENTHESIS},
				{.type = CYMB_TOKEN_CLOSE_PARENTHESIS}
			},
			.count = 2
		}, CYMB_DIRECTION_BACKWARD, CYMB_SUCCESS, 1, 0, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_FORWARD, CYMB_INVALID, 0, 0, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_BACKWARD, CYMB_INVALID, 2, 2, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_FORWARD, CYMB_SUCCESS, 0, 6, {}},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_DIRECTION_BACKWARD, CYMB_SUCCESS, 7, 1, {}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics1[] = {
		{
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = tests[1].tokens.tokens[0].info
		}
	};
	tests[1].diagnostics.start = diagnostics1;

	CymbDiagnostic diagnostics2[] = {
		{
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = tests[2].tokens.tokens[0].info
		}
	};
	tests[2].diagnostics.start = diagnostics2;

	CymbDiagnostic diagnostics5[] = {
		{
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = tests[5].tokens.tokens[0].info
		}
	};
	tests[5].diagnostics.start = diagnostics5;

	CymbDiagnostic diagnostics6[] = {
		{
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = tests[6].tokens.tokens[2].info
		}
	};
	tests[6].diagnostics.start = diagnostics6;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbArenaClear(&context->arena);
		cymbDiagnosticListFree(&context->diagnostics);

		cymbContextSetIndex(context, testIndex);

		size_t tokenIndex = tests[testIndex].startIndex;
		const CymbResult result = cymbSkipParentheses(&tests[testIndex].tokens, tests[testIndex].direction, &tokenIndex, &context->diagnostics);

		if(result != tests[testIndex].result)
		{
			cymbFail(context, "Wrong result.");
			continue;
		}

		if(result == CYMB_SUCCESS && tokenIndex != tests[testIndex].endIndex)
		{
			cymbFail(context, "Wrong token index.");
			continue;
		}

		if(result == CYMB_INVALID)
		{
			cymbCompareDiagnostics(&context->diagnostics, &tests[testIndex].diagnostics, context);
		}
	}

	cymbContextPop(context);
}

static void cymbTestExpressions(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbConstString test0String = CYMB_STRING("5 +");
	const CymbConstString test1String = CYMB_STRING("a");
	const CymbConstString test2String = CYMB_STRING("((5 * (26 + 27 * 28 + 29) + 37))");
	const CymbConstString test3String = CYMB_STRING("0 & 1 << 2 == 3 + 4 * 5 || 6 ^ 7 < 8 >> 9 / 10 && 11 - 12 >= 13 != 14 <= 15 | 16 > 17 % 18");
	const CymbConstString test4String = CYMB_STRING("-~0 * (*&a - ++!(--+b))");
	const CymbConstString test5String = CYMB_STRING("a += b * 5 * 3 = 1 + 2");
	const CymbConstString test6String = CYMB_STRING("a * (b + c");

	CymbTreeTest tests[] = {
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 1},
						.line = test0String,
						.hint = {test0String.string, 1}
					}
				},
				{.type = CYMB_TOKEN_PLUS}
			},
			.count = 2
		}, CYMB_INVALID, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 1},
						.line = test1String,
						.hint = test1String
					}
				}
			},
			.count = 1
		}, CYMB_SUCCESS, {}, {}, 1},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 17},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 37},
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_MINUS,
					.info = {
						.position = {1, 1},
						.line = test4String,
						.hint = {test4String.string + 0, 1}
					}
				},
				{
					.type = CYMB_TOKEN_TILDE,
					.info = {
						.position = {1, 2},
						.line = test4String,
						.hint = {test4String.string + 1, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 0},
					.info = {
						.position = {1, 3},
						.line = test4String,
						.hint = {test4String.string + 2, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 5},
						.line = test4String,
						.hint = {test4String.string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 7},
						.line = test4String,
						.hint = {test4String.string + 6, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 8},
						.line = test4String,
						.hint = {test4String.string + 7, 1}
					}
				},
				{
					.type = CYMB_TOKEN_AMPERSAND,
					.info = {
						.position = {1, 9},
						.line = test4String,
						.hint = {test4String.string + 8, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 10},
						.line = test4String,
						.hint = {test4String.string + 9, 1}
					}
				},
				{
					.type = CYMB_TOKEN_MINUS,
					.info = {
						.position = {1, 12},
						.line = test4String,
						.hint = {test4String.string + 11, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS_PLUS,
					.info = {
						.position = {1, 14},
						.line = test4String,
						.hint = {test4String.string + 13, 2}
					}
				},
				{
					.type = CYMB_TOKEN_EXCLAMATION,
					.info = {
						.position = {1, 16},
						.line = test4String,
						.hint = {test4String.string + 15, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 17},
						.line = test4String,
						.hint = {test4String.string + 16, 1}
					}
				},
				{
					.type = CYMB_TOKEN_MINUS_MINUS,
					.info = {
						.position = {1, 18},
						.line = test4String,
						.hint = {test4String.string + 17, 2}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 20},
						.line = test4String,
						.hint = {test4String.string + 19, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 21},
						.line = test4String,
						.hint = {test4String.string + 20, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 21},
						.line = test4String,
						.hint = {test4String.string + 20, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_PARENTHESIS,
					.info = {
						.position = {1, 22},
						.line = test4String,
						.hint = {test4String.string + 21, 1}
					}
				}
			},
			.count = 17
		}, CYMB_SUCCESS, {}, {}, 17},
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 1},
						.line = test5String,
						.hint = {test5String.string + 0, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS_EQUAL,
					.info = {
						.position = {1, 3},
						.line = test5String,
						.hint = {test5String.string + 2, 2}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 6},
						.line = test5String,
						.hint = {test5String.string + 5, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 8},
						.line = test5String,
						.hint = {test5String.string + 7, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 10},
						.line = test5String,
						.hint = {test5String.string + 9, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 12},
						.line = test5String,
						.hint = {test5String.string + 11, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 3},
					.info = {
						.position = {1, 14},
						.line = test5String,
						.hint = {test5String.string + 13, 1}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL,
					.info = {
						.position = {1, 16},
						.line = test5String,
						.hint = {test5String.string + 15, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 1},
					.info = {
						.position = {1, 18},
						.line = test5String,
						.hint = {test5String.string + 17, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 20},
						.line = test5String,
						.hint = {test5String.string + 19, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 2},
					.info = {
						.position = {1, 22},
						.line = test5String,
						.hint = {test5String.string + 21, 1}
					}
				}
			},
			.count = 11
		}, CYMB_SUCCESS, {}, {}, 11},
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 1},
						.line = test6String,
						.hint = {test6String.string + 0, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 3},
						.line = test6String,
						.hint = {test6String.string + 2, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_PARENTHESIS,
					.info = {
						.position = {1, 5},
						.line = test6String,
						.hint = {test6String.string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 6},
						.line = test6String,
						.hint = {test6String.string + 5, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 8},
						.line = test6String,
						.hint = {test6String.string + 7, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 10},
						.line = test6String,
						.hint = {test6String.string + 9, 1}
					}
				}
			},
			.count = 6
		}, CYMB_INVALID, {}, {}, 6}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbNode nodes0[] = {
		{
			.type = CYMB_NODE_CONSTANT,
			.constantNode = tests[0].tokens.tokens[0].constant,
			.info = tests[0].tokens.tokens[0].info
		}
	};
	tests[0].solution.root = nodes0 + 0;
	CymbDiagnostic diagnostics0[] = {
		{
			.type = CYMB_EXPECTED_EXPRESSION,
			.info = tests[0].tokens.tokens[1].info
		}
	};
	tests[0].diagnostics.start = diagnostics0;

	CymbNode nodes1[] = {
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[1].tokens.tokens[0].info
		}
	};
	tests[1].solution.root = nodes1 + 0;

	CymbNode nodes2[] = {
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes2[2],
				.rightNode = &nodes2[3]
			},
			.info = tests[2].tokens.tokens[8].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes2[1],
				.rightNode = &nodes2[4]
			},
			.info = tests[2].tokens.tokens[6].info
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.constantNode = {CYMB_CONSTANT_INT, 29},
			.info = tests[2].tokens.tokens[11].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes2[5],
				.rightNode = &nodes2[6]
			},
			.info = tests[2].tokens.tokens[10].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes2[0],
				.rightNode = &nodes2[7]
			},
			.info = tests[2].tokens.tokens[3].info
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.constantNode = {CYMB_CONSTANT_INT, 37},
			.info = tests[2].tokens.tokens[14].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes2[8],
				.rightNode = &nodes2[9]
			},
			.info = tests[2].tokens.tokens[13].info
		}
	};
	tests[2].solution.root = nodes2 + 10;

	CymbNode nodes3[] = {
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_LESSFT_SHIFT,
				.leftNode = &nodes3[1],
				.rightNode = &nodes3[2]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes3[5],
				.rightNode = &nodes3[6]
			},
			.info = tests[3].tokens.tokens[9].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes3[4],
				.rightNode = &nodes3[7]
			},
			.info = tests[3].tokens.tokens[7].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_EQUAL,
				.leftNode = &nodes3[3],
				.rightNode = &nodes3[8]
			},
			.info = tests[3].tokens.tokens[5].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_BITWISE_AND,
				.leftNode = &nodes3[0],
				.rightNode = &nodes3[9]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_DIVISION,
				.leftNode = &nodes3[14],
				.rightNode = &nodes3[15]
			},
			.info = tests[3].tokens.tokens[19].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_RIGHT_SHIFT,
				.leftNode = &nodes3[13],
				.rightNode = &nodes3[16]
			},
			.info = tests[3].tokens.tokens[17].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_LESS,
				.leftNode = &nodes3[12],
				.rightNode = &nodes3[17]
			},
			.info = tests[3].tokens.tokens[15].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_BITWISE_EXCLUSIVE_OR,
				.leftNode = &nodes3[11],
				.rightNode = &nodes3[18]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_SUBTRACTION,
				.leftNode = &nodes3[20],
				.rightNode = &nodes3[21]
			},
			.info = tests[3].tokens.tokens[23].info
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.constantNode = {CYMB_CONSTANT_INT, 13},
			.info = tests[3].tokens.tokens[26].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_GREATER_EQUAL,
				.leftNode = &nodes3[22],
				.rightNode = &nodes3[23]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_LESS_EQUAL,
				.leftNode = &nodes3[25],
				.rightNode = &nodes3[26]
			},
			.info = tests[3].tokens.tokens[29].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_NOT_EQUAL,
				.leftNode = &nodes3[24],
				.rightNode = &nodes3[27]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_REMAINDER,
				.leftNode = &nodes3[30],
				.rightNode = &nodes3[31]
			},
			.info = tests[3].tokens.tokens[35].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_GREATER,
				.leftNode = &nodes3[29],
				.rightNode = &nodes3[32]
			},
			.info = tests[3].tokens.tokens[33].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_BITWISE_OR,
				.leftNode = &nodes3[28],
				.rightNode = &nodes3[33]
			},
			.info = tests[3].tokens.tokens[31].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_LOGICAL_AND,
				.leftNode = &nodes3[19],
				.rightNode = &nodes3[34]
			},
			.info = tests[3].tokens.tokens[21].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_LOGICAL_OR,
				.leftNode = &nodes3[10],
				.rightNode = &nodes3[35]
			},
			.info = tests[3].tokens.tokens[11].info
		}
	};
	tests[3].solution.root = nodes3 + 36;

	CymbNode nodes4[] = {
		{
			.type = CYMB_NODE_CONSTANT,
			.info = tests[4].tokens.tokens[2].info,
			.constantNode = tests[4].tokens.tokens[2].constant
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[1].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_BITWISE_NOT,
				.node = &nodes4[0]
			}
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[0].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_NEGATIVE,
				.node = &nodes4[1]
			}
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[4].tokens.tokens[7].info
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[6].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_ADDRESS,
				.node = &nodes4[3]
			}
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[5].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_INDIRECTION,
				.node = &nodes4[4]
			}
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[4].tokens.tokens[14].info
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[13].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_POSITIVE,
				.node = &nodes4[6]
			}
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[12].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_DECREMENT,
				.node = &nodes4[7]
			}
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[10].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_LOGICAL_NOT,
				.node = &nodes4[8]
			}
		},
		{
			.type = CYMB_NODE_UNARY_OPERATOR,
			.info = tests[4].tokens.tokens[9].info,
			.unaryOperatorNode = {
				.operator = CYMB_UNARY_OPERATOR_INCREMENT,
				.node = &nodes4[9]
			}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[4].tokens.tokens[8].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_SUBTRACTION,
				.leftNode = &nodes4[5],
				.rightNode = &nodes4[10]
			}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[4].tokens.tokens[3].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes4[2],
				.rightNode = &nodes4[11]
			}
		}
	};
	tests[4].solution.root = nodes4 + 12;

	CymbNode nodes5[] = {
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[5].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[5].tokens.tokens[2].info
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.info = tests[5].tokens.tokens[4].info,
			.constantNode = {CYMB_CONSTANT_INT, 5}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[5].tokens.tokens[3].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes5[1],
				.rightNode = &nodes5[2]
			}
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.info = tests[5].tokens.tokens[6].info,
			.constantNode = {CYMB_CONSTANT_INT, 3}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[5].tokens.tokens[5].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_MULTIPLICATION,
				.leftNode = &nodes5[3],
				.rightNode = &nodes5[4]
			}
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.info = tests[5].tokens.tokens[8].info,
			.constantNode = {CYMB_CONSTANT_INT, 1}
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.info = tests[5].tokens.tokens[10].info,
			.constantNode = {CYMB_CONSTANT_INT, 2}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[5].tokens.tokens[9].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes5[6],
				.rightNode = &nodes5[7]
			}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[5].tokens.tokens[7].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ASSIGNMENT,
				.leftNode = &nodes5[5],
				.rightNode = &nodes5[8]
			}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[5].tokens.tokens[1].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION_ASSIGNMENT,
				.leftNode = &nodes5[0],
				.rightNode = &nodes5[9]
			}
		}
	};
	tests[5].solution.root = nodes5 + 10;

	CymbNode nodes6[] = {
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[6].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[6].tokens.tokens[3].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[6].tokens.tokens[5].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes6[1],
				.rightNode = &nodes6[2]
			},
			.info = tests[6].tokens.tokens[4].info
		}
	};
	tests[6].solution.root = nodes6 + 3;
	CymbDiagnostic diagnostics6[] = {
		{
			.type = CYMB_UNMATCHED_PARENTHESIS,
			.info = tests[6].tokens.tokens[2].info
		}
	};
	tests[6].diagnostics.start = diagnostics6;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseExpression, context);
	}

	cymbContextPop(context);
}

static void cymbTestTypes(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbConstString test0String = CYMB_STRING("int");
	const CymbConstString test1String = CYMB_STRING("float const*");
	const CymbConstString test2String = CYMB_STRING("const char* restrict* const");
	const CymbConstString test3String = CYMB_STRING("const const int const const");

	CymbTreeTest tests[] = {
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 1},
						.line = test2String,
						.hint = {test2String.string + 0, 5}
					}
				},
				{
					.type = CYMB_TOKEN_CHAR,
					.info = {
						.position = {1, 7},
						.line = test2String,
						.hint = {test2String.string + 6, 4}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 11},
						.line = test2String,
						.hint = {test2String.string + 10, 1}
					}
				},
				{
					.type = CYMB_TOKEN_RESTRICT,
					.info = {
						.position = {1, 13},
						.line = test2String,
						.hint = {test2String.string + 12, 8}
					}
				},
				{
					.type = CYMB_TOKEN_STAR,
					.info = {
						.position = {1, 21},
						.line = test2String,
						.hint = {test2String.string + 20, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONST,
					.info = {
						.position = {1, 23},
						.line = test2String,
						.hint = {test2String.string + 22, 5}
					}
				}
			},
			.count = 6
		}, CYMB_SUCCESS, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_INVALID, {}, {}, 0}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbNode nodes0[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_INT},
			.info = tests[0].tokens.tokens[0].info
		}
	};
	tests[0].solution.root = nodes0 + 0;

	CymbNode nodes1[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_FLOAT, .isConst = true},
			.info = tests[1].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_POINTER,
			.pointerNode = {.pointedNode = &nodes1[0]},
			.info = tests[1].tokens.tokens[2].info
		}
	};
	tests[1].solution.root = nodes1 + 1;

	CymbNode nodes2[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_CHAR, .isConst = true},
			.info = tests[2].tokens.tokens[1].info
		},
		{
			.type = CYMB_NODE_POINTER,
			.pointerNode = {.pointedNode = &nodes2[0], .isRestrict = true},
			.info = tests[2].tokens.tokens[2].info
		},
		{
			.type = CYMB_NODE_POINTER,
			.pointerNode = {.pointedNode = &nodes2[1], .isConst = true},
			.info = tests[2].tokens.tokens[4].info
		}
	};
	tests[2].solution.root = nodes2 + 2;

	CymbDiagnostic diagnostics3[] = {
		{
			.type = CYMB_MULTIPLE_CONST,
			.info = tests[3].tokens.tokens[1].info
		}
	};
	tests[3].diagnostics.start = diagnostics3;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseType, context);
	}

	cymbContextPop(context);
}

static void cymbTestStatements(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbConstString test0String = CYMB_STRING("return");
	const CymbConstString test2String = CYMB_STRING("return;");
	const CymbConstString test3String = CYMB_STRING("return 1 + 2; 3");
	const CymbConstString test4String = CYMB_STRING("int my_var;");
	const CymbConstString test5String = CYMB_STRING("const long other_var = 1;");
	const CymbConstString test6String = CYMB_STRING("while(a > 5){int b = 3; a += b + 5;}");
	const CymbConstString test7String = CYMB_STRING("while(0)return;return;");

	CymbTreeTest tests[] = {
		{{
			.tokens = (CymbToken[]){
				{
					.type = CYMB_TOKEN_RETURN,
					.info = {
						.position = {1, 1},
						.line = test0String,
						.hint = test0String
					}
				}
			},
			.count = 1
		}, CYMB_INVALID, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
				{.type = CYMB_TOKEN_SEMICOLON}
			},
			.count = 1
		}, CYMB_SUCCESS, {}, {}, 0},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 2},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 5},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 3},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 6},
		{{
			.tokens = (CymbToken[]){
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
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 25},
						.line = test6String,
						.hint = {test6String.string + 24, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS_EQUAL,
					.info = {
						.position = {1, 27},
						.line = test6String,
						.hint = {test6String.string + 26, 2}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 30},
						.line = test6String,
						.hint = {test6String.string + 29, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 32},
						.line = test6String,
						.hint = {test6String.string + 31, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {CYMB_CONSTANT_INT, 5},
					.info = {
						.position = {1, 34},
						.line = test6String,
						.hint = {test6String.string + 33, 1}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 35},
						.line = test6String,
						.hint = {test6String.string + 34, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CLOSE_BRACE,
					.info = {
						.position = {1, 36},
						.line = test6String,
						.hint = {test6String.string + 35, 1}
					}
				}
			},
			.count = 19
		}, CYMB_SUCCESS, {}, {}, 19},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 6}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics0[] = {
		{
			.type = CYMB_EXPECTED_SEMICOLON,
			.info = tests[0].tokens.tokens[0].info
		}
	};
	tests[0].diagnostics.start = diagnostics0;

	CymbNode nodes2[] = {
		{
			.type = CYMB_NODE_RETURN,
			.returnNode = nullptr,
			.info = tests[2].tokens.tokens[0].info
		}
	};
	tests[2].solution.root = nodes2 + 0;

	CymbNode nodes3[] = {
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes3[0],
				.rightNode = &nodes3[1]
			},
			.info = tests[3].tokens.tokens[2].info
		},
		{
			.type = CYMB_NODE_RETURN,
			.returnNode = &nodes3[2],
			.info = tests[3].tokens.tokens[0].info
		}
	};
	tests[3].solution.root = nodes3 + 3;

	CymbNode nodes4[] = {
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
			.declarationNode = {
				.type = &nodes4[0],
				.identifier = &nodes4[1]
			},
			.info = tests[4].tokens.tokens[1].info
		}
	};
	tests[4].solution.root = nodes4 + 2;

	CymbNode nodes5[] = {
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
			.declarationNode = {
				.type = &nodes5[0],
				.identifier = &nodes5[1],
				.initializer = &nodes5[2]
			},
			.info = tests[5].tokens.tokens[2].info
		}
	};
	tests[5].solution.root = nodes5 + 3;

	CymbNode nodes6[] = {
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_GREATER,
				.leftNode = &nodes6[0],
				.rightNode = &nodes6[1]
			},
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
			.declarationNode = {
				.type = &nodes6[3],
				.identifier = &nodes6[4],
				.initializer = &nodes6[5]
			},
			.info = tests[6].tokens.tokens[8].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[6].tokens.tokens[12].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[6].tokens.tokens[14].info
		},
		{
			.type = CYMB_NODE_CONSTANT,
			.constantNode = {CYMB_CONSTANT_INT, 5},
			.info = tests[6].tokens.tokens[16].info
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[6].tokens.tokens[15].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes6[8],
				.rightNode = &nodes6[9]
			}
		},
		{
			.type = CYMB_NODE_BINARY_OPERATOR,
			.info = tests[6].tokens.tokens[13].info,
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION_ASSIGNMENT,
				.leftNode = &nodes6[7],
				.rightNode = &nodes6[10]
			}
		},
		{
			.type = CYMB_NODE_WHILE,
			.whileNode = {
				.expression = &nodes6[2]
			},
			.info = tests[6].tokens.tokens[0].info
		}
	};
	CymbNodeChild children6[] = {
		{
			.node = &nodes6[6],
			.next = &children6[1]
		},
		{
			.node = &nodes6[11]
		}
	};
	nodes6[12].whileNode.body = children6 + 0;
	tests[6].solution.root = nodes6 + 12;

	CymbNode nodes7[] = {
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
			.whileNode = {
				.expression = &nodes7[0]
			},
			.info = tests[7].tokens.tokens[0].info
		}
	};
	CymbNodeChild children7[] = {
		{
			.node = &nodes7[1]
		}
	};
	nodes7[2].whileNode.body = children7 + 0;
	tests[7].solution.root = nodes7 + 2;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseStatement, context);
	}

	cymbContextPop(context);
}

static void cymbTestFunctions(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbConstString test0String = CYMB_STRING("int some_func(void){return 1 + 2; return; return 0;}");
	const CymbConstString test1String = CYMB_STRING("const float* some_other_func(){}");
	const CymbConstString test2String = CYMB_STRING("int func(){const int a = 1; return a + 2;};");
	const CymbConstString test3String = CYMB_STRING("void add(const int* const a, float b){return;}");

	CymbTreeTest tests[] = {
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 17},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 8},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 17},
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 16}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbNode nodes0[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_INT},
			.info = tests[0].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_FUNCTION_TYPE,
			.functionTypeNode = {
				.returnType = &nodes0[0]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes0[3],
				.rightNode = &nodes0[4]
			},
			.info = tests[0].tokens.tokens[8].info
		},
		{
			.type = CYMB_NODE_RETURN,
			.returnNode = &nodes0[5],
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
			.returnNode = &nodes0[8],
			.info = tests[0].tokens.tokens[13].info
		},
		{
			.type = CYMB_NODE_FUNCTION,
			.functionNode = {
				.type = &nodes0[1],
				.name = &nodes0[2]
			},
			.info = tests[0].tokens.tokens[1].info
		}
	};
	CymbNodeChild children0[] = {
		{
			.node = &nodes0[6],
			.next = &children0[1]
		},
		{
			.node = &nodes0[7],
			.next = &children0[2]
		},
		{
			.node = &nodes0[9]
		}
	};
	nodes0[10].functionNode.statements = children0 + 0;
	tests[0].solution.root = nodes0 + 10;

	CymbNode nodes1[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_FLOAT, .isConst = true},
			.info = tests[1].tokens.tokens[1].info
		},
		{
			.type = CYMB_NODE_POINTER,
			.pointerNode = {
				.pointedNode = &nodes1[0]
			},
			.info = tests[1].tokens.tokens[2].info
		},
		{
			.type = CYMB_NODE_FUNCTION_TYPE,
			.functionTypeNode = {
				.returnType = &nodes1[1]
			},
			.info = tests[1].tokens.tokens[2].info
		},
		{
			.type = CYMB_NODE_IDENTIFIER,
			.info = tests[1].tokens.tokens[3].info
		},
		{
			.type = CYMB_NODE_FUNCTION,
			.functionNode = {
				.type = &nodes1[2],
				.name = &nodes1[3]
			},
			.info = tests[1].tokens.tokens[3].info
		}
	};
	tests[1].solution.root = nodes1 + 4;

	CymbNode nodes2[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_INT},
			.info = tests[2].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_FUNCTION_TYPE,
			.functionTypeNode = {
				.returnType = &nodes2[0]
			},
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
			.declarationNode = {
				.type = &nodes2[3],
				.identifier = &nodes2[4],
				.initializer = &nodes2[5]
			},
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
			.binaryOperatorNode = {
				.operator = CYMB_BINARY_OPERATOR_ADDITION,
				.leftNode = &nodes2[7],
				.rightNode = &nodes2[8]
			},
			.info = tests[2].tokens.tokens[13].info
		},
		{
			.type = CYMB_NODE_RETURN,
			.returnNode = &nodes2[9],
			.info = tests[2].tokens.tokens[11].info
		},
		{
			.type = CYMB_NODE_FUNCTION,
			.functionNode = {
				.type = &nodes2[1],
				.name = &nodes2[2]
			},
			.info = tests[2].tokens.tokens[1].info
		}
	};
	CymbNodeChild children2[] = {
		{
			.node = &nodes2[6],
			.next = &children2[1]
		},
		{
			.node = &nodes2[10]
		}
	};
	nodes2[11].functionNode.statements = children2 + 0;
	tests[2].solution.root = nodes2 + 11;

	CymbNode nodes3[] = {
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
			.pointerNode = {.pointedNode = &nodes3[1], .isConst = true},
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
			.functionTypeNode = {
				.returnType = &nodes3[0]
			},
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
			.functionNode = {
				.type = &nodes3[6],
				.name = &nodes3[7]
			},
			.info = tests[3].tokens.tokens[1].info
		}
	};
	CymbNodeChild children3[] = {
		{
			.node = &nodes3[2],
			.next = &children3[1]
		},
		{
			.node = &nodes3[4]
		},
		{
			.node = &nodes3[3],
			.next = &children3[3]
		},
		{
			.node = &nodes3[5]
		},
		{
			.node = &nodes3[8]
		}
	};
	nodes3[6].functionTypeNode.parameterTypes = children3 + 0;
	nodes3[9].functionNode.parameters = children3 + 2;
	nodes3[9].functionNode.statements = children3 + 4;
	tests[3].solution.root = nodes3 + 9;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseFunction, context);
	}

	cymbContextPop(context);
}

static void cymbTestProgram(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbConstString test0String = CYMB_STRING("int some_func(){return 1;} int main(void){return 0;}");

	CymbTreeTest tests[] = {
		{{
			.tokens = (CymbToken[]){
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
		}, CYMB_SUCCESS, {}, {}, 19}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbNode nodes0[] = {
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_INT},
			.info = tests[0].tokens.tokens[0].info
		},
		{
			.type = CYMB_NODE_FUNCTION_TYPE,
			.functionTypeNode = {
				.returnType = &nodes0[0]
			},
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
			.returnNode = &nodes0[3],
			.info = tests[0].tokens.tokens[5].info
		},
		{
			.type = CYMB_NODE_FUNCTION,
			.functionNode = {
				.type = &nodes0[1],
				.name = &nodes0[2]
			},
			.info = tests[0].tokens.tokens[1].info
		},
		{
			.type = CYMB_NODE_TYPE,
			.typeNode = {.type = CYMB_TYPE_INT},
			.info = tests[0].tokens.tokens[9].info
		},
		{
			.type = CYMB_NODE_FUNCTION_TYPE,
			.functionTypeNode = {
				.returnType = &nodes0[6]
			},
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
			.returnNode = &nodes0[9],
			.info = tests[0].tokens.tokens[15].info
		},
		{
			.type = CYMB_NODE_FUNCTION,
			.functionNode = {
				.type = &nodes0[7],
				.name = &nodes0[8]
			},
			.info = tests[0].tokens.tokens[10].info
		},
		{
			.type = CYMB_NODE_PROGRAM,
			.programNode = {}
		}
	};
	CymbNodeChild children0[] = {
		{
			.node = &nodes0[4]
		},
		{
			.node = &nodes0[10]
		},
		{
			.node = &nodes0[5],
			.next = &children0[3]
		},
		{
			.node = &nodes0[11]
		}
	};
	nodes0[5].functionNode.statements = children0 + 0;
	nodes0[11].functionNode.statements = children0 + 1;
	nodes0[12].programNode.children = children0 + 2;
	tests[0].solution.root = nodes0 + 12;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoTreeTest(&tests[testIndex], cymbParseProgram, context);
	}

	cymbContextPop(context);
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
