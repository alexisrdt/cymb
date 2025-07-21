#ifndef CYMB_TREE_H
#define CYMB_TREE_H

#include <stddef.h>

#include "cymb/lex.h"

/*
 * A node type.
 */
typedef enum CymbNodeType
{
	CYMB_NODE_PROGRAM,
	CYMB_NODE_FUNCTION,
	CYMB_NODE_DECLARATION,
	CYMB_NODE_TYPE,
	CYMB_NODE_POINTER,
	CYMB_NODE_FUNCTION_TYPE,
	CYMB_NODE_WHILE,
	CYMB_NODE_RETURN,
	CYMB_NODE_BINARY_OPERATOR,
	CYMB_NODE_UNARY_OPERATOR,
	CYMB_NODE_IDENTIFIER,
	CYMB_NODE_CONSTANT
} CymbNodeType;

typedef struct CymbNode CymbNode;

/*
 * A list of child nodes.
 *
 * Fields:
 * - nodes: The children array.
 * - count: The number of children.
 */
typedef struct CymbNodeList
{
	CymbNode** nodes;
	size_t count;
} CymbNodeList;

/*
 * An object type.
 */
typedef enum CymbType
{
	CYMB_TYPE_VOID,
	CYMB_TYPE_CHAR,
	CYMB_TYPE_SIGNED_CHAR,
	CYMB_TYPE_UNSIGNED_CHAR,
	CYMB_TYPE_SHORT,
	CYMB_TYPE_UNSIGNED_SHORT,
	CYMB_TYPE_INT,
	CYMB_TYPE_UNSIGNED_INT,
	CYMB_TYPE_LONG,
	CYMB_TYPE_UNSIGNED_LONG,
	CYMB_TYPE_LONG_LONG,
	CYMB_TYPE_UNSIGNED_LONG_LONG,
	CYMB_TYPE_FLOAT,
	CYMB_TYPE_DOUBLE,
	CYMB_TYPE_BOOL
} CymbType;

/*
 * A type node.
 *
 * Fields:
 * - type: The type.
 * - isConst: A flag indicating if the type is const.
 * - isStatic: A flag indicating if the type is static.
 */
typedef struct CymbTypeNode
{
	CymbType type;

	bool isConst;
	bool isStatic;
} CymbTypeNode;

/*
 * A pointer node.
 *
 * Fields:
 * - pointedNode: The pointed type.
 * - isConst: A flag indicating if the pointer is const.
 * - isRestrict: A flag indicating if the pointer is restrict.
 */
typedef struct CymbPointerNode
{
	CymbNode* pointedNode;

	bool isConst;
	bool isRestrict;
} CymbPointerNode;

/*
 * A function type node.
 *
 * Fields:
 * - returnType: The return type.
 * - parameterTypesStart: The start of the parameter types array.
 * - parameterTypesCount: The number of parameter types.
 */
typedef struct CymbFunctionTypeNode
{
	CymbNode* returnType;

	CymbNodeList parameterTypes;
} CymbFunctionTypeNode;

/*
 * A program node.
 *
 * Fields:
 * - childrenStart: The start of the children array.
 * - childrenCount: The number of children.
 */
typedef struct CymbProgramNode
{
	CymbNodeList children;
} CymbProgramNode;

/*
 * A function node.
 *
 * Fields:
 * - name: The name.
 * - type: The type.
 * - parametersStart: The start of the parameters array.
 * - parametersCount: The number of parameters.
 * - statementsStart: The start of the statements array.
 * - statementsCount: The number of statements.
 */
typedef struct CymbFunctionNode
{
	CymbNode* name;
	CymbNode* type;

	CymbNodeList parameters;

	CymbNodeList statements;
} CymbFunctionNode;

/*
 * A declaration node.
 *
 * Fields:
 * - identifier: The identifier.
 * - type: The type.
 * - initializer: The initializer.
 */
typedef struct CymbDeclarationNode
{
	CymbNode* identifier;

	CymbNode* type;
	CymbNode* initializer;
} CymbDeclarationNode;

/*
 * A while node.
 *
 * Fields:
 * -
 */
typedef struct CymbWhileNode
{
	CymbNode* expression;
	CymbNodeList body;
} CymbWhileNode;

/*
 * A return node.
 */
typedef CymbNode* CymbReturnNode;

/*
 * A constant node.
 */
typedef CymbConstant CymbConstantNode;

/*
 * A binary operator.
 */
typedef enum CymbBinaryOperator
{
	CYMB_BINARY_OPERATOR_ADDITION,
	CYMB_BINARY_OPERATOR_SUBTRACTION,
	CYMB_BINARY_OPERATOR_MULTIPLICATION,
	CYMB_BINARY_OPERATOR_DIVISION,
	CYMB_BINARY_OPERATOR_REMAINDER,
	CYMB_BINARY_OPERATOR_LESSFT_SHIFT,
	CYMB_BINARY_OPERATOR_RIGHT_SHIFT,
	CYMB_BINARY_OPERATOR_LESS,
	CYMB_BINARY_OPERATOR_LESS_EQUAL,
	CYMB_BINARY_OPERATOR_GREATER,
	CYMB_BINARY_OPERATOR_GREATER_EQUAL,
	CYMB_BINARY_OPERATOR_EQUAL,
	CYMB_BINARY_OPERATOR_NOT_EQUAL,
	CYMB_BINARY_OPERATOR_BITWISE_AND,
	CYMB_BINARY_OPERATOR_BITWISE_EXCLUSIVE_OR,
	CYMB_BINARY_OPERATOR_BITWISE_OR,
	CYMB_BINARY_OPERATOR_LOGICAL_AND,
	CYMB_BINARY_OPERATOR_LOGICAL_OR,
	CYMB_BINARY_OPERATOR_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_ADDITION_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_SUBTRACTION_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_MULTIPLICATION_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_DIVISION_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_REMAINDER_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_LEFT_SHIFT_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_RIGHT_SHIFT_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_BITWISE_AND_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_BITWISE_EXCLUSIVE_OR_ASSIGNMENT,
	CYMB_BINARY_OPERATOR_BITWISE_OR_ASSIGNMENT
} CymbBinaryOperator;

/*
 * A binary operator node.
 *
 * Fields:
 * - operator: The operator.
 * - leftNode: The left operand.
 * - rightNode: The right operand.
 */
typedef struct CymbBinaryOperatorNode
{
	CymbBinaryOperator operator;

	CymbNode* leftNode;
	CymbNode* rightNode;
} CymbBinaryOperatorNode;

typedef enum CymbUnaryOperator
{
	CYMB_UNARY_OPERATOR_INCREMENT,
	CYMB_UNARY_OPERATOR_DECREMENT,
	CYMB_UNARY_OPERATOR_ADDRESS,
	CYMB_UNARY_OPERATOR_INDIRECTION,
	CYMB_UNARY_OPERATOR_POSITIVE,
	CYMB_UNARY_OPERATOR_NEGATIVE,
	CYMB_UNARY_OPERATOR_BITWISE_NOT,
	CYMB_UNARY_OPERATOR_LOGICAL_NOT
} CymbUnaryOperator;

typedef struct CymbUnaryOperatorNode
{
	CymbUnaryOperator operator;

	CymbNode* node;
} CymbUnaryOperatorNode;

/*
 * A node.
 *
 * Fields:
 * - type: The node type.
 * - info: The diagnostic info.
 * - programNode: The node data if it is a program node.
 * - functionNode: The node data if it is a function node.
 * - declarationNode: The node data if it is a declaration node.
 * - typeNode: The node data if it is a type node.
 * - pointerNode: The node data if it is a pointer node.
 * - functionTypeNode: The node data if it is a function type node.
 * - returnNode: The node data if it is a return node.
 * - binaryOperatorNode: The node data if it is a binary operator node.
 * - constantNode: The node data if it is a constant node.
 */
typedef struct CymbNode
{
	CymbNodeType type;

	CymbDiagnosticInfo info;

	union
	{
		CymbProgramNode programNode;
		CymbFunctionNode functionNode;
		CymbDeclarationNode declarationNode;
		CymbTypeNode typeNode;
		CymbPointerNode pointerNode;
		CymbFunctionTypeNode functionTypeNode;
		CymbWhileNode whileNode;
		CymbReturnNode returnNode;
		CymbBinaryOperatorNode binaryOperatorNode;
		CymbUnaryOperatorNode unaryOperatorNode;
		CymbConstantNode constantNode;
	};
} CymbNode;

/*
 * An abstract syntax tree.
 *
 * Fields:
 * - nodes: The nodes of the tree.
 * - children: The children nodes.
 * - count: The number of nodes.
 */
typedef struct CymbTree
{
	CymbNode* nodes;
	CymbNode** children;
	size_t count;
} CymbTree;

/*
 * A temporary structure holding objects necessary to build a tree.
 *
 * Fields:
 * - tree: A pointer to the tree to build.
 * - nodesCapacity: Number of items tree->nodes can store.
 * - childrenCapacity: Number of items tree->children can store.
 * - childCount: The number of children in the tree's children buffer.
 * - lastStored: The last child stored at the end of the children buffer.
 */
typedef struct CymbTreeBuilder
{
	CymbTree* tree;

	size_t nodesCapacity;
	size_t childrenCapacity;

	size_t childCount;
	size_t lastStored;
} CymbTreeBuilder;

/*
 * A traversal direction for skipping parentheses.
 */
typedef enum CymbDirection
{
	CYMB_DIRECTION_FORWARD = 1,
	CYMB_DIRECTION_BACKWARD = -1
} CymbDirection;

/*
 * Skip surrounding parentheses.
 *
 * Parameters:
 * - tokens: The tokens.
 * - direction: A traversal direction.
 * - parseResult: The result.
 * - tokenIndex: The starting index, updated to the ending index.
 * - diagnostics: A list of diagnostics.
 */
CymbResult cymbSkipParentheses(const CymbConstTokenList* tokens, CymbDirection direction, CymbParseResult* parseResult, size_t* tokenIndex, CymbDiagnosticList* diagnostics);

/*
 * A tree function.
 *
 * Parameters:
 * - builder: The builder tree.
 * - tokens: The tokens.
 * - parseResult: The result.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
typedef CymbResult (*CymbTreeFunction)(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse an expression
 */
CymbResult cymbParseExpression(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse a type.
 */
CymbResult cymbParseType(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse a statement.
 */
CymbResult cymbParseStatement(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse a function.
 */
CymbResult cymbParseFunction(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse a program.
 */
CymbResult cymbParseProgram(CymbTreeBuilder* builder, CymbConstTokenList* tokens, CymbParseResult* parseResult, CymbDiagnosticList* diagnostics);

/*
 * Parse tokens.
 *
 * Parameters:
 * - tokens: The tokens.
 * - tree: The tree.abort
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_ERROR_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
CymbResult cymbParse(const CymbConstTokenList* tokens, CymbTree* tree, CymbDiagnosticList* diagnostics);

/*
 * Free a tree.
 *
 * Parameters:
 * - tree: The tree.
 */
void cymbFreeTree(CymbTree* tree);

#endif
