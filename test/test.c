#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "cymb/memory.h"
#include "cymb/options.h"
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

	const struct
	{
		const CymbConstString* arguments;
		size_t argumentCount;
		CymbResult result;
		CymbOptions options;
		CymbConstDiagnosticList diagnostics;
	} tests[] = {
		{(const CymbConstString[]){
			CYMB_STRING("main.c")
		}, 1, CYMB_SUCCESS, {
			.inputs = (const char*[]){
				tests[0].arguments[0].string
			},
			.inputCount = 1,
			.standard = CYMB_C23,
			.tabWidth = 8
		}, {}},
		{(const CymbConstString[]){
			CYMB_STRING("-o"),
			CYMB_STRING("main.s"),
			CYMB_STRING("main.c"),
			CYMB_STRING("--output=-main.s-")
		}, 4, CYMB_SUCCESS, {
			.inputs = (const char*[]){
				tests[1].arguments[2].string
			},
			.inputCount = 1,
			.output = tests[1].arguments[3].string + 9,
			.standard = CYMB_C23,
			.tabWidth = 8
		}, {}},
		{(const CymbConstString[]){
			CYMB_STRING("--output")
		}, 1, CYMB_ERROR_INVALID_ARGUMENT, {}, {
			.diagnostics = (CymbDiagnostic[]){
				{
					.type = CYMB_MISSING_ARGUMENT,
					.info = {
						.hint = {tests[2].arguments[0].string + 2, tests[2].arguments[0].length - 2}
					}
				},
				{
					.type = CYMB_MISSING_ARGUMENT
				}
			},
			.count = 2
		}},
		{(const CymbConstString[]){
			CYMB_STRING("main.c"),
			CYMB_STRING("--some-option")
		}, 2, CYMB_ERROR_INVALID_ARGUMENT, {}, {
			.diagnostics = (CymbDiagnostic[]){
				{
					.type = CYMB_UNKNOWN_OPTION,
					.info = {
						.hint = {tests[3].arguments[1].string + 2, tests[3].arguments[1].length - 2}
					}
				}
			},
			.count = 1
		}},
		{(const CymbConstString[]){
			CYMB_STRING("--standard"),
			CYMB_STRING("c11"),
			CYMB_STRING("main.c"),
			CYMB_STRING("--tab-width=4")
		}, 4, CYMB_SUCCESS, {
			.inputs = (const char*[]){
				tests[4].arguments[2].string
			},
			.inputCount = 1,
			.tabWidth = 4,
			.standard = CYMB_C11
		}, {}},
		{(const CymbConstString[]){
			CYMB_STRING("main.c"),
			CYMB_STRING("--tab-width=1"),
			CYMB_STRING("--"),
			CYMB_STRING("--help"),
			CYMB_STRING("-v")
		}, 5, CYMB_SUCCESS, {
			.inputs = (const char*[]){
				tests[5].arguments[0].string,
				tests[5].arguments[3].string,
				tests[5].arguments[4].string
			},
			.inputCount = 3,
			.tabWidth = 1,
			.standard = CYMB_C23
		}, {}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		context->diagnostics.count = 0;

		CymbOptions options;
		const CymbResult result = cymbParseArguments(tests[testIndex].arguments, tests[testIndex].argumentCount, &options, &context->diagnostics);
		if(result != tests[testIndex].result)
		{
			cymbFail(context, "Wrong result.");
		}

		if(result == CYMB_SUCCESS)
		{
			if(options.debug != tests[testIndex].options.debug)
			{
				cymbFail(context, "Wrong debug.");
			}

			if(options.help != tests[testIndex].options.help)
			{
				cymbFail(context, "Wrong version.");
			}

			if(options.tabWidth != tests[testIndex].options.tabWidth)
			{
				cymbFail(context, "Wrong tab width.");
			}

			if(options.standard != tests[testIndex].options.standard)
			{
				cymbFail(context, "Wrong standard.");
			}

			if(options.inputCount != tests[testIndex].options.inputCount)
			{
				cymbFail(context, "Wrong input count.");
			}
			else
			{
				for(size_t inputIndex = 0; inputIndex < options.inputCount; ++inputIndex)
				{
					if(options.inputs[inputIndex] != tests[testIndex].options.inputs[inputIndex])
					{
						cymbFail(context, "Wrong input.");
					}
				}
			}

			if(options.output != tests[testIndex].options.output)
			{
				cymbFail(context, "Wrong output.");
			}
		}

		cymbCompareDiagnostics(&(const CymbConstDiagnosticList){
			.diagnostics = context->diagnostics.diagnostics,
			.count = context->diagnostics.count
		}, &tests[testIndex].diagnostics, context);
	}

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
