#ifndef CYMB_TEST_H
#define CYMB_TEST_H

#include "cymb/diagnostic.h"

typedef struct CymbTestContext
{
	bool passed;

	const char** strings;
	size_t stringCount;

	CymbDiagnosticList diagnostics;
} CymbTestContext;

void cymbFail(CymbTestContext* context, const char* string);

void cymbCompareDiagnosticInfo(const CymbDiagnosticInfo* first, const CymbDiagnosticInfo* second, CymbTestContext* context);
void cymbCompareDiagnostics(const CymbConstDiagnosticList* first, const CymbConstDiagnosticList* second, CymbTestContext* context);

void cymbTestLexs(CymbTestContext* context);
void cymbTestTrees(CymbTestContext* context);

#endif
