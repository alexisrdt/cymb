#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "cymb/arguments.h"
#include "cymb/memory.h"
#include "test.h"

void cymbFail(CymbTestContext* const context, const char* const string)
{
	context->passed = false;

	for(size_t stringIndex = 0; stringIndex < context->stringCount; ++stringIndex)
	{
		fputs(context->strings[stringIndex], stderr);
		fputc(stringIndex == context->stringCount - 1 ? ':' : ',', stderr);
		fputc(' ', stderr);
	}

	fputs(string, stderr);
	fputc('\n', stderr);
}

static void cymbTestTab(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const struct
	{
		size_t column;
		unsigned char tabWidth;
		size_t solution;
	} tests[] = {
		{1, 4, 5},
		{2, 4, 5},
		{3, 4, 5},
		{4, 4, 5},
		{5, 4, 9},
		{8, 4, 9},
		{9, 4, 13},
		{1, 2, 3},
		{2, 2, 3},
		{3, 2, 5},
		{1, 3, 4},
		{3, 3, 4},
		{4, 3, 7}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		const size_t result = cymbNextTab(tests[testIndex].column, tests[testIndex].tabWidth);
		if(result != tests[testIndex].solution)
		{
			snprintf(buffer, sizeof(buffer), format, __func__, testIndex);
			cymbFail(context, "Wrong result.");
		}
	}

	--context->stringCount;
}

static int cymbCompareInts(const void* const firstVoid, const void* const secondVoid)
{
	const int first = *(const int*)firstVoid;
	const int second = *(const int*)secondVoid;

	return (first > second) - (first < second);
}

static void cymbTestFind(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	constexpr int array[] = {1, 2, 3, 4, 5, 10, INT_MAX};
	constexpr size_t elementCount = CYMB_LENGTH(array);
	constexpr size_t elementSize = sizeof(array[0]);

	const int* result;

	for(size_t index = 0; index < elementCount; ++index)
	{
		result = cymbFind(&array[index], array, elementCount, elementSize, cymbCompareInts);
		if(result != &array[index])
		{
			snprintf(buffer, sizeof(buffer), format, __func__, index);
			cymbFail(context, "Wrong result.");
		}
	}

	result = cymbFind(&(const int){8}, array, elementCount, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, elementCount + 0);
		cymbFail(context, "Wrong result.");
	}

	result = cymbFind(&(const int){INT_MIN}, array, elementCount, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, elementCount + 1);
		cymbFail(context, "Wrong result.");
	}

	result = cymbFind(&array[0], array, 0, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, elementCount + 2);
		cymbFail(context, "Wrong result.");
	}

	--context->stringCount;
}

static void cymbTestArguments(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	CymbOptions options;

	snprintf(buffer, sizeof(buffer), format, __func__, (size_t)0);

	const char* const args1[] = {"-std=c"};
	if(cymbParseArguments(CYMB_LENGTH(args1), args1, &options) != CYMB_ERROR_INVALID_ARGUMENT)
	{
		cymbFail(context, "Wrong result.");
	}
	if(options.input || options.output)
	{
		cymbFail(context, "Wrong options.");
	}

	snprintf(buffer, sizeof(buffer), format, __func__, (size_t)1);

	const char* const args2[] = {"test.c"};
	if(cymbParseArguments(CYMB_LENGTH(args2), args2, &options) != CYMB_SUCCESS)
	{
		cymbFail(context, "Wrong result.");
	}

	if(!options.input || !options.output || options.debug || options.version != CYMB_C23)
	{
		cymbFail(context, "Wrong options.");
	}

	if(strcmp(options.input, args2[0]) != 0 || strcmp(options.output, "test.s") != 0)
	{
		cymbFail(context, "Wrong strings.");
	}

	free(options.input);
	free(options.output);

	snprintf(buffer, sizeof(buffer), format, __func__, (size_t)2);

	const char* const args3[] = {
		"test.c",
		"-o",
		"output.s",
		"-std=c23"
	};
	if(cymbParseArguments(CYMB_LENGTH(args3), args3, &options) != CYMB_SUCCESS)
	{
		cymbFail(context, "Wrong result.");
	}

	if(strcmp(options.input, args3[0]) != 0 || strcmp(options.output, "output.s") != 0 || options.version != CYMB_C23)
	{
		cymbFail(context, "Wrong options.");
	}

	free(options.input);
	free(options.output);

	--context->stringCount;
}

int main(void)
{
	const char* strings[16];
	CymbTestContext context = {.passed = true, .strings = strings};
	if(cymbDiagnosticListCreate(&context.diagnostics, "cymb_test", 4) != CYMB_SUCCESS)
	{
		fputs("Out of memory.\n", stderr);
		context.passed = false;
		goto end;
	}

	cymbTestTab(&context);

	cymbTestFind(&context);

	cymbTestArguments(&context);

	cymbTestLexs(&context);
	cymbTestTrees(&context);

	cymbDiagnosticListFree(&context.diagnostics);

	end:
	return context.passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
