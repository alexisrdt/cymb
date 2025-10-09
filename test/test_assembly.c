#include "cymb/assembly.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

typedef struct CymbAssemblyTest
{
	const CymbConstString assembly;
	bool success;
	uint32_t code;
	CymbDiagnosticList diagnostics;
} CymbAssemblyTest;

static void cymbDoAssemblyTest(const CymbAssemblyTest* const test, CymbTestContext* const context)
{
	const CymbArenaSave save = cymbArenaSave(&context->arena);

	uint32_t* codes;
	size_t count;

	CymbResult result = cymbAssemble(test->assembly.string, &codes, &count, &context->diagnostics);

	if(test->success)
	{
		if(result != CYMB_SUCCESS || count != 1 || codes[0] != test->code || context->diagnostics.start)
		{
			cymbFail(context, "Wrong result.");
		}

		free(codes);

		goto end;
	}

	if(result != CYMB_INVALID || codes != nullptr || count != 0)
	{
		cymbFail(context, "Wrong result.");
	}

	cymbCompareDiagnostics(&context->diagnostics, &test->diagnostics, context);

	end:
	cymbArenaRestore(&context->arena, save);
	cymbDiagnosticListFree(&context->diagnostics);
}

void cymbTestAssemblies(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbAssemblyTest tests[] = {
		// ABS
		{
			.assembly = CYMB_STRING("ABS X0, X1"),
			.success = true,
			.code = 0b1101'1010'1100'0000'0010'0000'0010'0000
		},
		{
			.assembly = CYMB_STRING("ABS W1, WZR"),
			.success = true,
			.code = 0b0101'1010'1100'0000'0010'0011'1110'0001
		},
		{
			.assembly = CYMB_STRING("ABS SP, X0"),
			.success = false,
			.diagnostics = {}
		},
		{
			.assembly = CYMB_STRING("ABS X0, SP"),
			.success = false,
			.diagnostics = {}
		},
		{
			.assembly = CYMB_STRING("ABS W0, X0"),
			.success = false,
			.diagnostics = {}
		},
		// ADC
		{
			.assembly = CYMB_STRING("ADC W10, W11, W12"),
			.success = true,
			.code = 0b0001'1010'0000'1100'0000'0001'0110'1010
		},
		// ADCS
		{
			.assembly = CYMB_STRING("ADCS X20, X21, X22"),
			.success = true,
			.code = 0b1011'1010'0001'0110'0000'0010'1011'0100
		},
		// ADD
		{
			.assembly = CYMB_STRING("ADD X0, X1, X2"),
			.success = true,
			.code = 0b1000'1011'0000'0010'0000'0000'0010'0000
		},
		{
			.assembly = CYMB_STRING("ADD X0, X1, W2, SXTH #1"),
			.success = true,
			.code = 0b1000'1011'0010'0010'1010'0100'0010'0000
		},
		{
			.assembly = CYMB_STRING("ADD W0, W1, #57"),
			.success = true,
			.code = 0b0001'0001'0000'0000'1110'0100'0010'0000
		},
		{
			.assembly = CYMB_STRING("ADD X0, X1, #4095, LSL #12"),
			.success = true,
			.code = 0b1001'0001'0111'1111'1111'1100'0010'0000
		},
		{
			.assembly = CYMB_STRING("ADD X0, X1, #4096"),
			.success = false,
			.diagnostics = {}
		}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics2[] = {
		{
			.type = CYMB_INVALID_SP,
			.info = {
				.position = {1, 5},
				.line = tests[2].assembly,
				.hint = {tests[2].assembly.string + 4, 2}
			}
		}
	};
	tests[2].diagnostics.start = diagnostics2;

	CymbDiagnostic diagnostics3[] = {
		{
			.type = CYMB_INVALID_SP,
			.info = {
				.position = {1, 9},
				.line = tests[3].assembly,
				.hint = {tests[3].assembly.string + 8, 2}
			}
		}
	};
	tests[3].diagnostics.start = diagnostics3;

	CymbDiagnostic diangostics4[] = {
		{
			.type = CYMB_INVALID_REGISTER_WIDTH,
			.info = {
				.position = {1, 9},
				.line = tests[4].assembly,
				.hint = {tests[4].assembly.string + 8, 2}
			}
		}
	};
	tests[4].diagnostics.start = diangostics4;

	CymbDiagnostic diagnostics11[] = {
		{
			.type = CYMB_INVALID_IMMEDIATE,
			.info = {
				.position = {1, 13},
				.line = tests[11].assembly,
				.hint = {tests[11].assembly.string + 12, 5}
			}
		}
	};
	tests[11].diagnostics.start = diagnostics11;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoAssemblyTest(&tests[testIndex], context);
	}

	cymbContextPop(context);
}
