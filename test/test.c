#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/memory.h"
#include "cymb/options.h"
#include "test.h"

void cymbContextPush(CymbTestContext* const context, const char* const string)
{
	CymbTestNode* const node = cymbArenaGet(&context->arena, sizeof(*node), alignof(typeof(*node)));
	if(!node)
	{
		context->passed = false;
		return;
	}

	*node = (CymbTestNode){
		.previous = context->lastNode,
		.save = cymbArenaSave(&context->arena),
		.string = string
	};

	if(context->lastNode)
	{
		context->lastNode->next = node;
	}
	context->lastNode = node;
	if(!context->firstNode)
	{
		context->firstNode = node;
	}
}

void cymbContextPop(CymbTestContext* const context)
{
	const CymbArenaSave save = context->lastNode->save;

	if(context->firstNode == context->lastNode)
	{
		context->firstNode = nullptr;
	}
	context->lastNode = context->lastNode->previous;
	if(context->lastNode)
	{
		context->lastNode->next = nullptr;
	}

	cymbArenaRestore(&context->arena, save);
}

void cymbContextSetIndex(CymbTestContext* const context, const size_t index)
{
	context->lastNode->index = index;
}

void cymbFail(CymbTestContext* const context, const char* const string)
{
	context->passed = false;

	const CymbTestNode* node = context->firstNode;
	while(node)
	{
		fprintf(stderr, "%s #%zu", node->string, node->index);
		fputc(node == context->lastNode ? ':' : ',', stderr);
		fputc(' ', stderr);

		node = node->next;
	}

	fputs(string, stderr);
	fputc('\n', stderr);
}

static void cymbTestTab(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

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
		cymbContextSetIndex(context, testIndex);

		const size_t result = cymbNextTab(tests[testIndex].column, tests[testIndex].tabWidth);
		if(result != tests[testIndex].solution)
		{
			cymbFail(context, "Wrong result.");
		}
	}

	cymbContextPop(context);
}

static int cymbCompareInts(const void* const firstVoid, const void* const secondVoid)
{
	const int first = *(const int*)firstVoid;
	const int second = *(const int*)secondVoid;

	return (first > second) - (first < second);
}

static void cymbTestFind(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	constexpr int array[] = {1, 2, 3, 4, 5, 10, INT_MAX};
	constexpr size_t elementCount = CYMB_LENGTH(array);
	constexpr size_t elementSize = sizeof(array[0]);

	const int* result;

	for(size_t index = 0; index < elementCount; ++index)
	{
		cymbContextSetIndex(context, index);

		result = cymbFind(&array[index], array, elementCount, elementSize, cymbCompareInts);
		if(result != &array[index])
		{
			cymbFail(context, "Wrong result.");
		}
	}

	cymbContextSetIndex(context, elementCount);
	result = cymbFind(&(const int){8}, array, elementCount, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		cymbFail(context, "Wrong result.");
	}

	cymbContextSetIndex(context, elementCount + 1);
	result = cymbFind(&(const int){INT_MIN}, array, elementCount, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		cymbFail(context, "Wrong result.");
	}

	cymbContextSetIndex(context, elementCount + 2);
	result = cymbFind(&array[0], array, 0, elementSize, cymbCompareInts);
	if(result != nullptr)
	{
		cymbFail(context, "Wrong result.");
	}

	cymbContextPop(context);
}

static void cymbTestArguments(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	struct
	{
		const CymbConstString* arguments;
		size_t argumentCount;
		CymbResult result;
		CymbOptions options;
		CymbDiagnosticList diagnostics;
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
		}, 1, CYMB_INVALID, {}, {}},
		{(const CymbConstString[]){
			CYMB_STRING("main.c"),
			CYMB_STRING("--some-option")
		}, 2, CYMB_INVALID, {}, {}},
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
		}, {}},
		{nullptr, 0, CYMB_INVALID, {}, {}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics2[] = {
		{
			.type = CYMB_MISSING_ARGUMENT,
			.info = {
				.hint = {tests[2].arguments[0].string + 2, tests[2].arguments[0].length - 2}
			},
			.next = diagnostics2 + 1
		},
		{
			.type = CYMB_MISSING_ARGUMENT
		}
	};
	tests[2].diagnostics.start = diagnostics2;

	CymbDiagnostic diagnostics3[] = {
		{
			.type = CYMB_UNKNOWN_OPTION,
			.info = {
				.hint = {tests[3].arguments[1].string + 2, tests[3].arguments[1].length - 2}
			}
		}
	};
	tests[3].diagnostics.start = diagnostics3;

	CymbDiagnostic diagnostics6[] = {
		{
			.type = CYMB_MISSING_ARGUMENT
		}
	};
	tests[6].diagnostics.start = diagnostics6;

	const CymbArenaSave save = cymbArenaSave(&context->arena);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

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

		cymbCompareDiagnostics(&context->diagnostics, &tests[testIndex].diagnostics, context);

		if(options.inputCount > 0)
		{
			free(options.inputs);
		}

		cymbArenaRestore(&context->arena, save);
		cymbDiagnosticListFree(&context->diagnostics);
	}

	cymbContextPop(context);
}

static void cymbTestMurmur3(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const struct
	{
		CymbConstString string;
		uint32_t solution;
	} tests[] = {
		{CYMB_STRING(""), 0x00000000},
		{CYMB_STRING("test"), 0xBA6BD213},
		{CYMB_STRING("Hello, world!"), 0xC0363E43},
		{CYMB_STRING("cymb"), 0xF5188C8F},
		{CYMB_STRING("Cymb"), 0xED4CCC41}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++ testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		const uint32_t hash = cymbMurmur3((const unsigned char*)tests[testIndex].string.string, tests[testIndex].string.length);

		if(hash != tests[testIndex].solution)
		{
			cymbFail(context, "Wrong hash.");
		}
	}

	cymbContextPop(context);
}

static void cymbTestMap(CymbTestContext* const context)
{
	const CymbArenaSave save = cymbArenaSave(&context->arena);

	CymbMap map;
	if(cymbMapCreate(&map, &context->arena, 16, sizeof(unsigned int), alignof(unsigned int)) != CYMB_SUCCESS)
	{
		context->passed = false;
		return;
	}

	char keys[26 * 26 * 2];

	for(char first = 'a'; first <= 'z'; ++first)
	{
		for(char second = 'a'; second <= 'z'; ++second)
		{
			const unsigned char firstOffset = first - 'a';
			const unsigned char secondOffset = second - 'a';

			const unsigned int offset = firstOffset * 26 + secondOffset;

			keys[offset * 2 + 0] = first;
			keys[offset * 2 + 1] = second;

			const CymbStringView key = {keys + offset * 2, 2};

			if(cymbMapStore(&map, key, &offset) != CYMB_SUCCESS)
			{
				goto error;
			}
		}
	}

	for(char first = 'a'; first <= 'z'; ++first)
	{
		for(char second = 'a'; second <= 'z'; ++second)
		{
			const unsigned char firstOffset = first - 'a';
			const unsigned char secondOffset = second - 'a';

			const unsigned int offset = firstOffset * 26 + secondOffset;

			const CymbStringView key = {keys + offset * 2, 2};

			const unsigned int* const result = cymbMapRead(&map, key);
			if(!result || *result != offset)
			{
				goto error;
			}
		}
	}
	
	if(cymbMapRead(&map, (CymbConstString)CYMB_STRING("other_key")))
	{
		goto error;
	}

	goto end;

	error:
	context->passed = false;

	end:
	cymbMapFree(&map);
	cymbArenaRestore(&context->arena, save);
}

int main(void)
{
	CymbTestContext context = {.passed = true};
	if(cymbArenaCreate(&context.arena) != CYMB_SUCCESS)
	{
		context.passed = false;
		goto end;
	}
	cymbDiagnosticListCreate(&context.diagnostics, &context.arena, "cymb_test", 4);

	cymbTestTab(&context);

	cymbTestFind(&context);

	cymbTestArguments(&context);

	cymbTestMurmur3(&context);

	cymbTestMap(&context);

	cymbTestLexs(&context);
	cymbTestTrees(&context);
	cymbTestAssemblies(&context);

	cymbArenaFree(&context.arena);

	end:
	return context.passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
