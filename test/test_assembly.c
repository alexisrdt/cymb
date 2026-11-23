#include "cymb/assembly.h"

#include <stdint.h>
#include <string.h>

#include "test.h"

typedef struct CymbAssemblyTest
{
	const CymbConstString assembly;
	bool success;
	const void* codes;
	size_t codeSize;
	const CymbDiagnostic* diagnostics;
	size_t diagnosticCount;
} CymbAssemblyTest;

static void cymbDoAssemblyTest(const CymbAssemblyTest* const test, const CymbMachine machine, CymbTestContext* const context)
{
	const CymbArenaSave save = cymbArenaSave(&context->arena);

	cymbSegmentedArrayCreate(&context->diagnostics.diagnostics, &context->arena, sizeof(CymbDiagnostic), alignof(CymbDiagnostic));

	char* codes;
	size_t size;

	CymbResult result = cymbAssemble(test->assembly.string, &codes, &size, machine, &context->diagnostics);

	if(test->success)
	{
		if(result != CYMB_SUCCESS || size != test->codeSize || memcmp(codes, test->codes, size) != 0 || context->diagnostics.diagnostics.end)
		{
			cymbFail(context, "Wrong result.");
		}

		goto end;
	}

	if(result != CYMB_INVALID || codes != nullptr || size != 0)
	{
		cymbFail(context, "Wrong result.");
	}

	cymbCompareDiagnostics(&context->diagnostics, test->diagnostics, test->diagnosticCount, context);

	end:
	cymbArenaRestore(&context->arena, save);
}

static void cymbTestX8664Assemblies(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbAssemblyTest tests[] = {
		// ret
		{
			.assembly = CYMB_STRING("ret"),
			.success = true,
			.codes = (const unsigned char[]){0xc3},
			.codeSize = 1
		},
		{
			.assembly = CYMB_STRING("ret 1"),
			.success = true,
			.codes = (const unsigned char[]){0xc2, 0x01, 0x00},
			.codeSize = 3
		}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoAssemblyTest(&tests[testIndex], CYMB_X86_64, context);
	}

	cymbContextPop(context);
}

static void cymbTestAarch64Assemblies(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbAssemblyTest tests[] = {
		// ret
		{
			.assembly = CYMB_STRING("ret"),
			.success = true,
			.codes = (const unsigned char[]){0xc0, 0x03, 0x5f, 0xd6},
			.codeSize = sizeof(uint32_t)
		},
		{
			.assembly = CYMB_STRING("ret x12"),
			.success = true,
			.codes = (const unsigned char[]){0x80, 0x01, 0x5f, 0xd6},
			.codeSize = sizeof(uint32_t)
		}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoAssemblyTest(&tests[testIndex], CYMB_AARCH64, context);
	}

	cymbContextPop(context);
}

void cymbTestAssemblies(CymbTestContext* const context)
{
	cymbTestX8664Assemblies(context);
	cymbTestAarch64Assemblies(context);
}
