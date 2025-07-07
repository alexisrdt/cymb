#include "cymb/lex.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "test.h"

typedef struct CymbLexTest
{
	const char* string;
	CymbParseResult result;
	CymbToken solution;
	CymbConstDiagnosticList diagnostics;
	CymbReader reader;
} CymbLexTest;

void cymbCompareDiagnosticInfo(const CymbDiagnosticInfo* const first, const CymbDiagnosticInfo* const second, CymbTestContext* const context)
{
	if(first->position.line != second->position.line || first->position.column != second->position.column)
	{
		cymbFail(context, "Wrong position.");
	}

	if(first->line.string != second->line.string || first->line.length != second->line.length)
	{
		cymbFail(context, "Wrong line.");
	}

	if(first->hint.string != second->hint.string || first->hint.length != second->hint.length)
	{
		cymbFail(context, "Wrong hint.");
	}
}

static void cymbCompareTokens(const CymbToken* const first, const CymbToken* const second, CymbTestContext* const context)
{
	if(first->type != second->type)
	{
		cymbFail(context, "Wrong token type.");
	}
	else if(first->type == CYMB_TOKEN_CONSTANT)
	{
		if(first->constant.type != second->constant.type)
		{
			cymbFail(context, "Wrong constant type.");
		}

		if(first->constant.value != second->constant.value)
		{
			cymbFail(context, "Wrong constant value.");
		}
	}

	cymbCompareDiagnosticInfo(&first->info, &second->info, context);
}

void cymbCompareDiagnostics(const CymbConstDiagnosticList* const first, const CymbConstDiagnosticList* const second, CymbTestContext* const context)
{
	if(first->count != second->count)
	{
		cymbFail(context, "Wrong number of diagnostics.");
		return;
	}

	const char* const format = "diagnostic #%zu";
	char buffer[64];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	for(size_t diagnosticIndex = 0; diagnosticIndex < first->count; ++diagnosticIndex)
	{
		snprintf(buffer, sizeof(buffer), format, diagnosticIndex);

		if(first->diagnostics[diagnosticIndex].type != second->diagnostics[diagnosticIndex].type)
		{
			cymbFail(context, "Wrong diagnostic type.");
		}

		cymbCompareDiagnosticInfo(&first->diagnostics[diagnosticIndex].info, &second->diagnostics[diagnosticIndex].info, context);
	}

	--context->stringCount;
}

static void cymbDoLexTest(const CymbLexTest* const test, const CymbLexFunction testFunction, CymbTestContext* const context)
{
	context->diagnostics.count = 0;

	CymbReader reader;
	cymbReaderCreate(test->string, context->diagnostics.tabWidth, &reader);

	CymbParseResult parseResult;
	CymbToken token;

	const CymbResult result = testFunction(&reader, &parseResult, &token, &context->diagnostics);
	if(result != CYMB_SUCCESS)
	{
		cymbFail(context, "Wrong result.");
		return;
	}

	if(parseResult != test->result)
	{
		cymbFail(context, "Wrong parse result.");
		return;
	}

	if(reader.string != test->reader.string)
	{
		cymbFail(context, "Wrong reader string.");
	}
	if(reader.position.line != test->reader.position.line || reader.position.column != test->reader.position.column)
	{
		cymbFail(context, "Wrong reader position.");
	}
	if(reader.line.string != test->reader.line.string || reader.line.length != test->reader.line.length)
	{
		cymbFail(context, "Wrong reader line.");
	}

	if(parseResult == CYMB_PARSE_NO_MATCH)
	{
		return;
	}

	cymbCompareTokens(&token, &test->solution, context);
	cymbCompareDiagnostics(&(const CymbConstDiagnosticList){
		.diagnostics = context->diagnostics.diagnostics,
		.count = context->diagnostics.count
	}, &test->diagnostics, context);
}

static void cymbTestStrings(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "123", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 3}
		}},
		{.string = "+=", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[3].string,
			.position = {1, 1},
			.line = {tests[3].string, 2}
		}},
		{.string = "\"string\"", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[4].string, 8},
				.hint = {tests[4].string, 8}
			}
		}, .reader = {
			.string = tests[4].string + 8,
			.position = {1, 9},
			.line = {tests[4].string, 8}
		}},
		{.string = "\"st\x01ri\nng\"", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[5].string, 6},
				.hint = {tests[5].string, 6}
			}
		}, .diagnostics = {
			.count = 2,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_INVALID_STRING_CHARACTER,
					.info = {
						.position = {1, 4},
						.line = {tests[5].string, 6},
						.hint = {tests[5].string + 3, 1}
					}
				},
				{
					.type = CYMB_UNFINISHED_STRING,
					.info = {
						.position = {1, 1},
						.line = {tests[5].string, 6},
						.hint = {tests[5].string, 6}
					}
				}
			}
		}, .reader = {
			.string = tests[5].string + 6,
			.position = {1, 7},
			.line = {tests[5].string, 6}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseString, context);
	}

	--context->stringCount;
}

static void cymbTestCharacters(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "123", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 3}
		}},
		{.string = "+=", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[3].string,
			.position = {1, 1},
			.line = {tests[3].string, 2}
		}},
		{.string = "\"string\"", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[4].string,
			.position = {1, 1},
			.line = {tests[4].string, 8}
		}},
		{.string = "'c'", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {
				.type = CYMB_CONSTANT_INT,
				.value = 'c'
			},
			.info = {
				.position = {1, 1},
				.line = {tests[5].string, 3},
				.hint = {tests[5].string, 3}
			}
		}, .reader = {
			.string = tests[5].string + 3,
			.position = {1, 4},
			.line = {tests[5].string, 3}
		}},
		{.string = "'a\n'", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {
				.type = CYMB_CONSTANT_INT,
				.value = 'a'
			},
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 2},
				.hint = {tests[6].string, 2}
			}
		}, .diagnostics = {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_INVALID_CHARACTER_CONSTANT,
					.info = {
						.position = {1, 1},
						.line = {tests[6].string, 2},
						.hint = {tests[6].string, 2}
					}
				}
			}
		}, .reader = {
			.string = tests[6].string + 2,
			.position = {1, 3},
			.line = {tests[6].string, 2}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseCharacter, context);
	}

	--context->stringCount;
}

static void cymbTestTokens(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "+=", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_PLUS_EQUAL,
			.info = {
				.position = {1, 1},
				.line = {tests[1].string, 2},
				.hint = {tests[1].string, 2}
			}
		}, .reader = {
			.string = tests[1].string + 2,
			.position = {1, 3},
			.line = {tests[1].string, 2}
		}},
		{.string = "+++", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_PLUS_PLUS,
			.info = {
				.position = {1, 1},
				.line = {tests[2].string, 3},
				.hint = {tests[2].string, 2}
			}
		}, .reader = {
			.string = tests[2].string + 2,
			.position = {1, 3},
			.line = {tests[2].string, 3}
		}},
		{.string = "+-", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_PLUS,
			.info = {
				.position = {1, 1},
				.line = {tests[3].string, 2},
				.hint = {tests[3].string, 1}
			}
		}, .reader = {
			.string = tests[3].string + 1,
			.position = {1, 2},
			.line = {tests[3].string, 2}
		}},
		{.string = "[{()}]", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_OPEN_BRACKET,
			.info = {
				.position = {1, 1},
				.line = {tests[4].string, 6},
				.hint = {tests[4].string, 1}
			}
		}, .reader = {
			.string = tests[4].string + 1,
			.position = {1, 2},
			.line = {tests[4].string, 6}
		}},
		{.string = "abc", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[5].string,
			.position = {1, 1},
			.line = {tests[5].string, 3}
		}},
		{.string = "123", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[6].string,
			.position = {1, 1},
			.line = {tests[6].string, 3}
		}},
		{.string = "\"string\"", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 8}
		}},
		{.string = "'c'", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[8].string,
			.position = {1, 1},
			.line = {tests[8].string, 3}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseToken, context);
	}

	--context->stringCount;
}

static void cymbTestConstants(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "[]", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 2}
		}},
		{.string = "57", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 57},
			.info = {
				.position = {1, 1},
				.line = {tests[3].string, 2},
				.hint = {tests[3].string, 2}
			}
		}, .reader = {
			.string = tests[3].string + 2,
			.position = {1, 3},
			.line = {tests[3].string, 2}
		}},
		{.string = "57llu", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_UNSIGNED_LONG_LONG, .value = 57},
			.info = {
				.position = {1, 1},
				.line = {tests[4].string, 5},
				.hint = {tests[4].string, 5}
			}
		}, .reader = {
			.string = tests[4].string + 5,
			.position = {1, 6},
			.line = {tests[4].string, 5}
		}},
		{.string = "0x42L", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_LONG, .value = 0x42},
			.info = {
				.position = {1, 1},
				.line = {tests[5].string, 5},
				.hint = {tests[5].string, 5}
			}
		}, .reader = {
			.string = tests[5].string + 5,
			.position = {1, 6},
			.line = {tests[5].string, 5}
		}},
		{.string = "0 a", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 3},
				.hint = {tests[6].string, 1}
			}
		}, .reader = {
			.string = tests[6].string + 1,
			.position = {1, 2},
			.line = {tests[6].string, 3}
		}},
		{.string = " 1", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 2}
		}},
		{.string = "0xyz", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
			.info = {
				.position = {1, 1},
				.line = {tests[8].string, 4},
				.hint = {tests[8].string, 4}
			}
		}, .diagnostics = {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_INVALID_CONSTANT_SUFFIX,
					.info = {
						.position = {1, 2},
						.line = {tests[8].string, 4},
						.hint = {tests[8].string + 1, 3}
					}
				}
			}
		}, .reader = {
			.string = tests[8].string + 4,
			.position = {1, 5},
			.line = {tests[8].string, 4}
		}},
		{.string = "0'1'23'4", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 01234},
			.info = {
				.position = {1, 1},
				.line = {tests[9].string, 8},
				.hint = {tests[9].string, 8}
			}
		}, .reader = {
			.string = tests[9].string + 8,
			.position = {1, 9},
			.line = {tests[9].string, 8}
		}},
		{.string = "0x'12''3'''4'\n", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0x1234},
			.info = {
				.position = {1, 1},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string, 13}
			}
		}, .diagnostics = {
			.count = 5,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_SEPARATOR_AFTER_BASE,
					.info = {
						.position = {1, 3},
						.line = {tests[10].string, 13},
						.hint = {tests[10].string + 2, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 7},
						.line = {tests[10].string, 13},
						.hint = {tests[10].string + 6, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 10},
						.line = {tests[10].string, 13},
						.hint = {tests[10].string + 9, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 11},
						.line = {tests[10].string, 13},
						.hint = {tests[10].string + 10, 1}
					}
				},
				{
					.type = CYMB_TRAILING_SEPARATOR,
					.info = {
						.position = {1, 13},
						.line = {tests[10].string, 13},
						.hint = {tests[10].string + 12, 1}
					}
				}
			}
		}, .reader = {
			.string = tests[10].string + 13,
			.position = {1, 14},
			.line = {tests[10].string, 13}
		}},
		{.string = "0b'''101'''lu", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_UNSIGNED_LONG, .value = 0b101},
			.info = {
				.position = {1, 1},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string, 13}
			}
		}, .diagnostics = {
			.count = 6,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_SEPARATOR_AFTER_BASE,
					.info = {
						.position = {1, 3},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 2, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 4},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 3, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 5},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 4, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 10},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 9, 1}
					}
				},
				{
					.type = CYMB_DUPLICATE_SEPARATOR,
					.info = {
						.position = {1, 11},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 10, 1}
					}
				},
				{
					.type = CYMB_TRAILING_SEPARATOR,
					.info = {
						.position = {1, 11},
						.line = {tests[11].string, 13},
						.hint = {tests[11].string + 10, 1}
					}
				}
			}
		}, .reader = {
			.string = tests[11].string + 13,
			.position = {1, 14},
			.line = {tests[11].string, 13}
		}},
		{.string = "0b''", .result = CYMB_PARSE_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
			.info = {
				.position = {1, 1},
				.line = {tests[12].string, 4},
				.hint = {tests[12].string, 2}
			}
		}, .diagnostics = {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_INVALID_CONSTANT_SUFFIX,
					.info = {
						.position = {1, 2},
						.line = {tests[12].string, 4},
						.hint = {tests[12].string + 1, 1}
					}
				}
			}
		}, .reader = {
			.string = tests[12].string + 2,
			.position = {1, 3},
			.line = {tests[12].string, 4}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseConstant, context);
	}

	--context->stringCount;
}

static void cymbTestIdentifiers(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "int", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_INT,
			.info = {
				.position = {1, 1},
				.line = {tests[1].string, 3},
				.hint = {tests[1].string, 3}
			}
		}, .reader = {
			.string = tests[1].string + 3,
			.position = {1, 4},
			.line = {tests[1].string, 3}
		}},
		{.string = "int5a", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_IDENTIFIER,
			.info = {
				.position = {1, 1},
				.line = {tests[2].string, 5},
				.hint = {tests[2].string, 5},
			}
		}, .reader = {
			.string = tests[2].string + 5,
			.position = {1, 6},
			.line = {tests[2].string, 5}
		}},
		{.string = "int_t", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_IDENTIFIER,
			.info = {
				.position = {1, 1},
				.line = {tests[3].string, 5},
				.hint = {tests[3].string, 5},
			}
		}, .reader = {
			.string = tests[3].string + 5,
			.position = {1, 6},
			.line = {tests[3].string, 5}
		}},
		{.string = " int", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[4].string,
			.position = {1, 1},
			.line = {tests[4].string, 4}
		}},
		{.string = "7a84de", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[5].string,
			.position = {1, 1},
			.line = {tests[5].string, 6}
		}},
		{.string = "_my_var_", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_IDENTIFIER,
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 8},
				.hint = {tests[6].string, 8},
			}
		}, .reader = {
			.string = tests[6].string + 8,
			.position = {1, 9},
			.line = {tests[6].string, 8}
		}},
		{.string = "[float]", .result = CYMB_PARSE_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 7}
		}},
		{.string = "fl;oat", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_IDENTIFIER,
			.info = {
				.position = {1, 1},
				.line = {tests[8].string, 6},
				.hint = {tests[8].string, 2},
			}
		}, .reader = {
			.string = tests[8].string + 2,
			.position = {1, 3},
			.line = {tests[8].string, 6}
		}},
		{.string = "float{}", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_FLOAT,
			.info = {
				.position = {1, 1},
				.line = {tests[9].string, 7},
				.hint = {tests[9].string, 5}
			}
		}, .reader = {
			.string = tests[9].string + 5,
			.position = {1, 6},
			.line = {tests[9].string, 7}
		}},
		{.string = "do int", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_DO,
			.info = {
				.position = {1, 1},
				.line = {tests[10].string, 6},
				.hint = {tests[10].string, 2}
			}
		}, .reader = {
			.string = tests[10].string + 2,
			.position = {1, 3},
			.line = {tests[10].string, 6}
		}},
		{.string = "double", .result = CYMB_PARSE_MATCH, .solution = {
			.type = CYMB_TOKEN_DOUBLE,
			.info = {
				.position = {1, 1},
				.line = {tests[11].string, 6},
				.hint = {tests[11].string, 6}
			}
		}, .reader = {
			.string = tests[11].string + 6,
			.position = {1, 7},
			.line = {tests[11].string, 6}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseIdentifier, context);
	}

	--context->stringCount;
}

static void cymbTestLex(CymbTestContext* const context)
{
	const char* const format = "%s #%zu";
	char buffer[32];
	context->strings[context->stringCount] = buffer;
	++context->stringCount;

	const struct
	{
		const char* string;
		CymbConstTokenList tokens;
		bool valid;
		CymbConstDiagnosticList diagnostics;
	} tests[] = {
		{.string = "", .tokens = {
			.tokens = nullptr,
			.count = 0
		}, .valid = true},
		{.string = "a + b", .tokens = {
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 1},
						.line = {tests[1].string, 5},
						.hint = {tests[1].string, 1}
					}
				},
				{
					.type = CYMB_TOKEN_PLUS,
					.info = {
						.position = {1, 3},
						.line = {tests[1].string, 5},
						.hint = {tests[1].string + 2, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = {tests[1].string, 5},
						.hint = {tests[1].string + 4, 1}
					}
				}
			},
			.count = 3
		}, .valid = true},
		{.string = "i-*=p[\"s\"'c'5ul/x", .tokens = {
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 1},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string, 1}
					}
				},
				{
					.type = CYMB_TOKEN_MINUS,
					.info = {
						.position = {1, 2},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 1, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STAR_EQUAL,
					.info = {
						.position = {1, 3},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 2, 2}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_OPEN_BRACKET,
					.info = {
						.position = {1, 6},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 5, 1}
					}
				},
				{
					.type = CYMB_TOKEN_STRING,
					.info = {
						.position = {1, 7},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 6, 3}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {.type = CYMB_CONSTANT_INT, .value = 'c'},
					.info = {
						.position = {1, 10},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 9, 3}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {.type = CYMB_CONSTANT_UNSIGNED_LONG, .value = 5},
					.info = {
						.position = {1, 13},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 12, 3}
					}
				},
				{
					.type = CYMB_TOKEN_SLASH,
					.info = {
						.position = {1, 16},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 15, 1}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 17},
						.line = {tests[2].string, 17},
						.hint = {tests[2].string + 16, 1}
					}
				}
			},
			.count = 10
		}, .valid = true},
		{.string = "int a = 0xyz;", .tokens = {
			.tokens = (const CymbToken[]){
				{
					.type = CYMB_TOKEN_INT,
					.info = {
						.position = {1, 1},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string, 3}
					}
				},
				{
					.type = CYMB_TOKEN_IDENTIFIER,
					.info = {
						.position = {1, 5},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string + 4, 1}
					}
				},
				{
					.type = CYMB_TOKEN_EQUAL,
					.info = {
						.position = {1, 7},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string + 6, 1}
					}
				},
				{
					.type = CYMB_TOKEN_CONSTANT,
					.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
					.info = {
						.position = {1, 9},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string + 8, 4}
					}
				},
				{
					.type = CYMB_TOKEN_SEMICOLON,
					.info = {
						.position = {1, 13},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string + 12, 1}
					}
				}
			},
			.count = 5
		}, .valid = false, .diagnostics = {
			.count = 1,
			.diagnostics = (const CymbDiagnostic[]){
				{
					.type = CYMB_INVALID_CONSTANT_SUFFIX,
					.info = {
						.position = {1, 10},
						.line = {tests[3].string, 13},
						.hint = {tests[3].string + 9, 3}
					}
				}
			}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		context->diagnostics.count = 0;

		snprintf(buffer, sizeof(buffer), format, __func__, testIndex);

		CymbTokenList tokens;
		const CymbResult result = cymbLex(tests[testIndex].string, &tokens, &context->diagnostics);

		if((tests[testIndex].valid && result != CYMB_SUCCESS) || (!tests[testIndex].valid && result != CYMB_ERROR_INVALID_ARGUMENT))
		{
			cymbFail(context, "Wrong result.");
			continue;
		}

		cymbCompareDiagnostics(&(const CymbConstDiagnosticList){
			.diagnostics = context->diagnostics.diagnostics,
			.count = context->diagnostics.count
		}, &tests[testIndex].diagnostics, context);

		if(tokens.count != tests[testIndex].tokens.count)
		{
			cymbFail(context, "Wrong token count.");
			free(tokens.tokens);
			continue;
		}

		const char* const tokenFormat = "token #%zu";
		char tokenBuffer[32];
		context->strings[context->stringCount] = tokenBuffer;
		++context->stringCount;

		for(size_t tokenIndex = 0; tokenIndex < tokens.count; ++tokenIndex)
		{
			snprintf(tokenBuffer, sizeof(tokenBuffer), tokenFormat, tokenIndex);

			cymbCompareTokens(&tokens.tokens[tokenIndex], &tests[testIndex].tokens.tokens[tokenIndex], context);
		}
		
		--context->stringCount;

		free(tokens.tokens);
	}

	--context->stringCount;
}

void cymbTestLexs(CymbTestContext* const context)
{
	cymbTestStrings(context);
	cymbTestCharacters(context);
	cymbTestTokens(context);
	cymbTestConstants(context);
	cymbTestIdentifiers(context);
	cymbTestLex(context);
}
