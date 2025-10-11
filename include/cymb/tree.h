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
	CYMB_NODE_CONSTANT,
	CYMB_NODE_FUNCTION_CALL,
	CYMB_NODE_ARRAY_SUBSCRIPT,
	CYMB_NODE_MEMBER_ACCESS,
	CYMB_NODE_POSTFIX_OPERATOR
} CymbNodeType;

typedef struct CymbNode CymbNode;

/*
 * A child node.
 *
 * Fields:
 * - node: The child node.
 * - next: The next child.
 */
typedef struct CymbNodeChild
{
	CymbNode* node;
	struct CymbNodeChild* next;
} CymbNodeChild;

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

	bool isConst: 1;
	bool isStatic: 1;
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

	bool isConst: 1;
	bool isRestrict: 1;
} CymbPointerNode;

/*
 * A function type node.
 *
 * Fields:
 * - returnType: The return type.
 * - parameterTypesStart: The parameter types.
 */
typedef struct CymbFunctionTypeNode
{
	CymbNode* returnType;

	CymbNodeChild* parameterTypes;
} CymbFunctionTypeNode;

/*
 * A program node.
 *
 * Fields:
 * - children: The children.
 */
typedef struct CymbProgramNode
{
	CymbNodeChild* children;
} CymbProgramNode;

/*
 * A function node.
 *
 * Fields:
 * - name: The name.
 * - type: The type.
 * - parameters: The parameters.
 * - statements: The statements array.
 */
typedef struct CymbFunctionNode
{
	CymbNode* name;
	CymbNode* type;

	CymbNodeChild* parameters;

	CymbNodeChild* statements;
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
 * - expression: The controlling expression.
 * - body: The body.
 */
typedef struct CymbWhileNode
{
	CymbNode* expression;
	CymbNodeChild* body;
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

/*
 * A unary operator.
 */
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

/*
 * A unary operator node.
 *
 * Fields:
 * - operator: The operator.
 * - node: The operand.
 */
typedef struct CymbUnaryOperatorNode
{
	CymbUnaryOperator operator;

	CymbNode* node;
} CymbUnaryOperatorNode;

/*
 * A function call node.
 *
 * Fields:
 * - name: The function name.
 * - arguments: The arguments.
 */
typedef struct CymbFunctionCallNode
{
	CymbNode* name;

	CymbNodeChild* arguments;
} CymbFunctionCallNode;

/*
 * An array subscript node.
 *
 * Fields:
 * - name: The array name.
 * - expression: The subscripting expression.
 */
typedef struct CymbArraySubscriptNode
{
	CymbNode* name;

	CymbNode* expression;
} CymbArraySubscriptNode;

/*
 * A member access type.
 */
typedef enum CymbMemberAccessType
{
	CYMB_MEMBER_ACCESS,
	CYMB_MEMBER_ACCESS_POINTER
} CymbMemberAccessType;

/*
 * A member access node.
 *
 * Fields:
 * - type: The member access type.
 * - name: The object name.
 * - member: The member name.
 */
typedef struct CymbMemberAccessNode
{
	CymbMemberAccessType type;

	CymbNode* name;
	CymbNode* member;
} CymbMemberAccessNode;

/*
 * A postfix operator.
 */
typedef enum CymbPostfixOperator
{
	CYMB_POSTFIX_OPERATOR_INCREMENT,
	CYMB_POSTFIX_OPERATOR_DECREMENT
} CymbPostfixOperator;

/*
 * A postfix operator node.
 *
 * Fields:
 * - operator: The operator.
 * - node: The operand.
 */
typedef struct CymbPostfixOperatorNode
{
	CymbPostfixOperator operator;

	CymbNode* node;
} CymbPostfixOperatorNode;

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
 * - whileNode: The node data if it is a while node.
 * - returnNode: The node data if it is a return node.
 * - binaryOperatorNode: The node data if it is a binary operator node.
 * - unaryOperatorNode: The node data if it is a unary operator node.
 * - constantNode: The node data if it is a constant node.
 * - functionCallNode: The node data if it is a function call node.
 * - arraySubscriptNode: The node data if it is an array subscript node.
 * - memberAccessNode: The node data if it is a member access node.
 * - postfixOperatorNode: The node data if it is a postfix operator node.
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
		CymbFunctionCallNode functionCallNode;
		CymbArraySubscriptNode arraySubscriptNode;
		CymbMemberAccessNode memberAccessNode;
		CymbPostfixOperatorNode postfixOperatorNode;
	};
} CymbNode;

/*
 * An abstract syntax tree.
 *
 * Fields:
 * - arena: The arena to use for allocations.
 * - root: The root of the tree.
 */
typedef struct CymbTree
{
	CymbArena* arena;
	CymbNode* root;
} CymbTree;

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
 * - tokenIndex: The starting index, updated to the ending index.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_NO_MATCH if the starting token is not a parenthesis.
 * - CYMB_INVALID if the parentheses pattern is invalid.
 */
CymbResult cymbSkipParentheses(const CymbTokenList* tokens, CymbDirection direction, size_t* tokenIndex, CymbDiagnosticList* diagnostics);

/*
 * A tree function.
 *
 * Parameters:
 * - tree: The tree.
 * - tokens: The tokens.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
typedef CymbResult (*CymbTreeFunction)(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse an expression
 */
CymbResult cymbParseExpression(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse a type.
 */
CymbResult cymbParseType(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse a statement.
 */
CymbResult cymbParseStatement(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse a function.
 */
CymbResult cymbParseFunction(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse a program.
 */
CymbResult cymbParseProgram(CymbTree* tree, CymbTokenList* tokens, CymbDiagnosticList* diagnostics);

/*
 * Parse tokens.
 *
 * Parameters:
 * - tokens: The tokens.
 * - arena: The arena to use for allocations.
 * - tree: The tree.abort
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if a node or diagnostic could not be added.
 */
CymbResult cymbParse(const CymbTokenList* tokens, CymbArena* arena, CymbTree* tree, CymbDiagnosticList* diagnostics);

/*
 * Free a tree.
 *
 * Parameters:
 * - tree: The tree.
 */
void cymbFreeTree(CymbTree* tree);

#endif
