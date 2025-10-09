#include "cymb/lex.h"

#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

typedef struct CymbLexTest
{
	const char* string;
	CymbResult result;
	CymbToken solution;
	CymbDiagnosticList diagnostics;
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

void cymbCompareDiagnostics(const CymbDiagnosticList* const first, const CymbDiagnosticList* const second, CymbTestContext* const context)
{
	cymbContextPush(context, "diagnostic");

	const CymbDiagnostic* firstDiagnostic = first->start;
	const CymbDiagnostic* secondDiagnostic = second->start;

	size_t diagnosticIndex = 0;
	while(firstDiagnostic)
	{
		cymbContextSetIndex(context, diagnosticIndex);

		if(!secondDiagnostic)
		{
			cymbFail(context, "Unexpected diagnostic.");
			goto end;
		}

		if(firstDiagnostic->type != secondDiagnostic->type)
		{
			cymbFail(context, "Wrong diagnostic type.");
		}

		cymbCompareDiagnosticInfo(&firstDiagnostic->info, &secondDiagnostic->info, context);

		firstDiagnostic = firstDiagnostic->next;
		secondDiagnostic = secondDiagnostic->next;

		++diagnosticIndex;
	}

	if(secondDiagnostic)
	{
		cymbContextSetIndex(context, diagnosticIndex);
		cymbFail(context, "Missing diagnostic.");
	}

	end:
	cymbContextPop(context);
}

static void cymbDoLexTest(const CymbLexTest* const test, const CymbLexFunction testFunction, CymbTestContext* const context)
{
	const CymbArenaSave save = cymbArenaSave(&context->arena);

	CymbReader reader;
	cymbReaderCreate(test->string, context->diagnostics.tabWidth, &reader);

	CymbToken token = {
		.info = {
			.position = reader.position,
			.line = reader.line,
			.hint.string = reader.string
		}
	};

	const CymbResult result = testFunction(&reader, &token, &context->diagnostics);

	if(result != test->result)
	{
		cymbFail(context, "Wrong result.");
		goto end;
	}

	if(result != CYMB_SUCCESS && result != CYMB_NO_MATCH && result != CYMB_INVALID)
	{
		goto end;
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

	if(result == CYMB_NO_MATCH)
	{
		goto end;
	}

	cymbCompareTokens(&token, &test->solution, context);
	cymbCompareDiagnostics(&context->diagnostics, &test->diagnostics, context);

	end:
	cymbArenaRestore(&context->arena, save);
	cymbDiagnosticListFree(&context->diagnostics);
}

static void cymbTestStrings(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbLexTest tests[] = {
		{.string = "", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "123", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 3}
		}},
		{.string = "+=", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[3].string,
			.position = {1, 1},
			.line = {tests[3].string, 2}
		}},
		{.string = "\"string\"", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "\"st\x01ri\nng\"", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[5].string, 6},
				.hint = {tests[5].string, 6}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[5].string + 6,
			.position = {1, 7},
			.line = {tests[5].string, 6}
		}},
		{.string = "\"string\\\"", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 9},
				.hint = {tests[6].string, 9}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[6].string + 9,
			.position = {1, 10},
			.line = {tests[6].string, 9}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics5[] = {
		{
			.type = CYMB_INVALID_STRING_CHARACTER,
			.info = {
				.position = {1, 4},
				.line = {tests[5].string, 6},
				.hint = {tests[5].string + 3, 1}
			},
			.next = diagnostics5 + 1
		},
		{
			.type = CYMB_UNFINISHED_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[5].string, 6},
				.hint = {tests[5].string, 6}
			}
		}
	};
	tests[5].diagnostics.start = diagnostics5;

	CymbDiagnostic diagnostics6[] = {
		{
			.type = CYMB_UNFINISHED_STRING,
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 9},
				.hint = {tests[6].string, 9}
			}
		}
	};
	tests[6].diagnostics.start = diagnostics6;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseString, context);
	}

	cymbContextPop(context);
}

static void cymbTestCharacters(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbLexTest tests[] = {
		{.string = "", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "123", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 3}
		}},
		{.string = "+=", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[3].string,
			.position = {1, 1},
			.line = {tests[3].string, 2}
		}},
		{.string = "\"string\"", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[4].string,
			.position = {1, 1},
			.line = {tests[4].string, 8}
		}},
		{.string = "'c'", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "'a\n'", .result = CYMB_INVALID, .solution = {
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
		}, .diagnostics = {}, .reader = {
			.string = tests[6].string + 2,
			.position = {1, 3},
			.line = {tests[6].string, 2}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics6[] = {
		{
			.type = CYMB_INVALID_CHARACTER_CONSTANT,
			.info = {
				.position = {1, 1},
				.line = {tests[6].string, 2},
				.hint = {tests[6].string, 2}
			}
		}
	};
	tests[6].diagnostics.start = diagnostics6;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseCharacter, context);
	}

	cymbContextPop(context);
}

static void cymbTestPunctuators(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "+=", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "+++", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "+-", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "[{()}]", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "abc", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[5].string,
			.position = {1, 1},
			.line = {tests[5].string, 3}
		}},
		{.string = "123", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[6].string,
			.position = {1, 1},
			.line = {tests[6].string, 3}
		}},
		{.string = "\"string\"", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 8}
		}},
		{.string = "'c'", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[8].string,
			.position = {1, 1},
			.line = {tests[8].string, 3}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParsePunctuator, context);
	}

	cymbContextPop(context);
}

static void cymbTestConstants(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	CymbLexTest tests[] = {
		{.string = "", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "abc", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[1].string,
			.position = {1, 1},
			.line = {tests[1].string, 3}
		}},
		{.string = "[]", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[2].string,
			.position = {1, 1},
			.line = {tests[2].string, 2}
		}},
		{.string = "57", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "57llu", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "0x42L", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "0 a", .result = CYMB_SUCCESS, .solution = {
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
		{.string = " 1", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 2}
		}},
		{.string = "0xyz", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
			.info = {
				.position = {1, 1},
				.line = {tests[8].string, 4},
				.hint = {tests[8].string, 4}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[8].string + 4,
			.position = {1, 5},
			.line = {tests[8].string, 4}
		}},
		{.string = "0'1'23'4", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "0x'12''3'''4'\n", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0x1234},
			.info = {
				.position = {1, 1},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string, 13}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[10].string + 13,
			.position = {1, 14},
			.line = {tests[10].string, 13}
		}},
		{.string = "0b'''101'''lu", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_UNSIGNED_LONG, .value = 0b101},
			.info = {
				.position = {1, 1},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string, 13}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[11].string + 13,
			.position = {1, 14},
			.line = {tests[11].string, 13}
		}},
		{.string = "0b''", .result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 0},
			.info = {
				.position = {1, 1},
				.line = {tests[12].string, 4},
				.hint = {tests[12].string, 2}
			}
		}, .diagnostics = {}, .reader = {
			.string = tests[12].string + 2,
			.position = {1, 3},
			.line = {tests[12].string, 4}
		}},
		{.string = "'1'", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[13].string,
			.position = {1, 1},
			.line = {tests[13].string, 3}
		}},
		{.result = CYMB_INVALID, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {CYMB_CONSTANT_INT, 0}
		}},
		{.string = "011", .result = CYMB_SUCCESS, .solution = {
			.type = CYMB_TOKEN_CONSTANT,
			.constant = {.type = CYMB_CONSTANT_INT, .value = 9},
			.info = {
				.position = {1, 1},
				.line = {tests[15].string, 3},
				.hint = {tests[15].string, 3}
			}
		}, .reader = {
			.string = tests[15].string + 3,
			.position = {1, 4},
			.line = {tests[15].string, 3}
		}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics8[] = {
		{
			.type = CYMB_INVALID_CONSTANT_SUFFIX,
			.info = {
				.position = {1, 2},
				.line = {tests[8].string, 4},
				.hint = {tests[8].string + 1, 3}
			}
		}
	};
	tests[8].diagnostics.start = diagnostics8;

	CymbDiagnostic diagnostics10[] = {
		{
			.type = CYMB_SEPARATOR_AFTER_BASE,
			.info = {
				.position = {1, 3},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string + 2, 1}
			},
			.next = diagnostics10 + 1
		},
		{
			.type = CYMB_DUPLICATE_SEPARATORS,
			.info = {
				.position = {1, 6},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string + 5, 2}
			},
			.next = diagnostics10 + 2
		},
		{
			.type = CYMB_DUPLICATE_SEPARATORS,
			.info = {
				.position = {1, 9},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string + 8, 3}
			},
			.next = diagnostics10 + 3
		},
		{
			.type = CYMB_TRAILING_SEPARATOR,
			.info = {
				.position = {1, 13},
				.line = {tests[10].string, 13},
				.hint = {tests[10].string + 12, 1}
			}
		}
	};
	tests[10].diagnostics.start = diagnostics10;
	
	CymbDiagnostic diagnostics11[] = {
		{
			.type = CYMB_SEPARATOR_AFTER_BASE,
			.info = {
				.position = {1, 3},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string + 2, 1}
			},
			.next = diagnostics11 + 1
		},
		{
			.type = CYMB_DUPLICATE_SEPARATORS,
			.info = {
				.position = {1, 3},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string + 2, 3}
			},
			.next = diagnostics11 + 2
		},
		{
			.type = CYMB_DUPLICATE_SEPARATORS,
			.info = {
				.position = {1, 9},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string + 8, 3}
			},
			.next = diagnostics11 + 3
		},
		{
			.type = CYMB_TRAILING_SEPARATOR,
			.info = {
				.position = {1, 11},
				.line = {tests[11].string, 13},
				.hint = {tests[11].string + 10, 1}
			}
		}
	};
	tests[11].diagnostics.start = diagnostics11;

	CymbDiagnostic diagnostics12[] = {
		{
			.type = CYMB_INVALID_CONSTANT_SUFFIX,
			.info = {
				.position = {1, 2},
				.line = {tests[12].string, 4},
				.hint = {tests[12].string + 1, 1}
			}
		}
	};
	tests[12].diagnostics.start = diagnostics12;

	const char* const format14 = "%"PRIuMAX"%"PRIuMAX;
	const int length14 = snprintf(nullptr, 0, format14, UINTMAX_MAX, UINTMAX_MAX);
	if(length14 <= 0 || length14 >= INT_MAX)
	{
		context->passed = false;
		goto end;
	}
	char* const string14 = malloc(length14 + 1);
	if(!string14)
	{
		context->passed = false;
		goto end;
	}
	if(snprintf(string14, length14 + 1, format14, UINTMAX_MAX, UINTMAX_MAX) != length14)
	{
		context->passed = false;
		goto clear;
	}
	tests[14].string = string14;
	tests[14].solution.info = (CymbDiagnosticInfo){
		.position = {1, 1},
		.line = {string14, length14},
		.hint = {string14, length14}
	};
	tests[14].reader = (CymbReader){
		.string = string14 + length14,
		.position = {1, length14 + 1},
		.line = {string14, length14}
	};
	CymbDiagnostic diagnostics14[] = {
		{
			.type = CYMB_CONSTANT_TOO_LARGE,
			.info = {
				.position = {1, 1},
				.line = tests[14].reader.line,
				.hint = {tests[14].string, length14}
			}
		}
	};
	tests[14].diagnostics.start = diagnostics14;

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseConstant, context);
	}

	clear:
	free(string14);

	end:
	cymbContextPop(context);
}

static void cymbTestIdentifiers(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	const CymbLexTest tests[] = {
		{.string = "", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[0].string,
			.position = {1, 1},
			.line = {tests[0].string, 0}
		}},
		{.string = "int", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "int5a", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "int_t", .result = CYMB_SUCCESS, .solution = {
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
		{.string = " int", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[4].string,
			.position = {1, 1},
			.line = {tests[4].string, 4}
		}},
		{.string = "7a84de", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[5].string,
			.position = {1, 1},
			.line = {tests[5].string, 6}
		}},
		{.string = "_my_var_", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "[float]", .result = CYMB_NO_MATCH, .reader = {
			.string = tests[7].string,
			.position = {1, 1},
			.line = {tests[7].string, 7}
		}},
		{.string = "fl;oat", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "float{}", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "do int", .result = CYMB_SUCCESS, .solution = {
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
		{.string = "double", .result = CYMB_SUCCESS, .solution = {
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
		cymbContextSetIndex(context, testIndex);

		cymbDoLexTest(&tests[testIndex], cymbParseIdentifier, context);
	}

	cymbContextPop(context);
}

static void cymbTestLex(CymbTestContext* const context)
{
	cymbContextPush(context, __func__);

	struct
	{
		const char* string;
		CymbTokenList tokens;
		bool valid;
		CymbDiagnosticList diagnostics;
	} tests[] = {
		{.string = "", .tokens = {
			.tokens = nullptr,
			.count = 0
		}, .valid = true},
		{.string = "a + b", .tokens = {
			.tokens = (CymbToken[]){
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
			.tokens = (CymbToken[]){
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
			.tokens = (CymbToken[]){
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
		}, .valid = false, .diagnostics = {}}
	};
	constexpr size_t testCount = CYMB_LENGTH(tests);

	CymbDiagnostic diagnostics3[] = {
		{
			.type = CYMB_INVALID_CONSTANT_SUFFIX,
			.info = {
				.position = {1, 10},
				.line = {tests[3].string, 13},
				.hint = {tests[3].string + 9, 3}
			}
		}
	};
	tests[3].diagnostics.start = diagnostics3;

	const CymbArenaSave save = cymbArenaSave(&context->arena);

	for(size_t testIndex = 0; testIndex < testCount; ++testIndex)
	{
		cymbContextSetIndex(context, testIndex);

		CymbTokenList tokens;
		const CymbResult result = cymbLex(tests[testIndex].string, &tokens, &context->diagnostics);

		if((tests[testIndex].valid && result != CYMB_SUCCESS) || (!tests[testIndex].valid && result != CYMB_INVALID))
		{
			cymbFail(context, "Wrong result.");
			goto next;
		}

		cymbCompareDiagnostics(&context->diagnostics, &tests[testIndex].diagnostics, context);

		if(tokens.count != tests[testIndex].tokens.count)
		{
			cymbFail(context, "Wrong token count.");
			free(tokens.tokens);
			goto next;
		}

		cymbContextPush(context, "token");

		for(size_t tokenIndex = 0; tokenIndex < tokens.count; ++tokenIndex)
		{
			cymbContextSetIndex(context, tokenIndex);

			cymbCompareTokens(&tokens.tokens[tokenIndex], &tests[testIndex].tokens.tokens[tokenIndex], context);
		}
		
		cymbContextPop(context);

		cymbFreeTokenList(&tokens);

		next:
		cymbArenaRestore(&context->arena, save);
		cymbDiagnosticListFree(&context->diagnostics);
	}

	cymbContextPop(context);
}

void cymbTestLexs(CymbTestContext* const context)
{
	cymbTestStrings(context);
	cymbTestCharacters(context);
	cymbTestPunctuators(context);
	cymbTestConstants(context);
	cymbTestIdentifiers(context);
	cymbTestLex(context);
}
