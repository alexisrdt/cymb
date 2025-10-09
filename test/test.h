#ifndef CYMB_TEST_H
#define CYMB_TEST_H

#include "cymb/diagnostic.h"

typedef struct CymbTestNode
{
	struct CymbTestNode* previous;
	struct CymbTestNode* next;

	CymbArenaSave save;

	const char* string;
	size_t index;
} CymbTestNode;

typedef struct CymbTestContext
{
	bool passed;

	CymbTestNode* firstNode;
	CymbTestNode* lastNode;

	CymbArena arena;
	CymbDiagnosticList diagnostics;
} CymbTestContext;

void cymbContextPush(CymbTestContext* context, const char* string);
void cymbContextPop(CymbTestContext* context);

void cymbContextSetIndex(CymbTestContext* context, size_t index);

void cymbFail(CymbTestContext* context, const char* string);

void cymbCompareDiagnosticInfo(const CymbDiagnosticInfo* first, const CymbDiagnosticInfo* second, CymbTestContext* context);
void cymbCompareDiagnostics(const CymbDiagnosticList* first, const CymbDiagnosticList* second, CymbTestContext* context);

void cymbTestLexs(CymbTestContext* context);
void cymbTestTrees(CymbTestContext* context);
void cymbTestAssemblies(CymbTestContext* context);

#endif
