#include "cymb/assembly.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cymb/lex.h"

/*
 * Parameters:
 * - Zs: GPR or ZR, shift s.
 * - Ss: GPR or SP, shift s.
 * - Es,o,i: Extended register, shift s, option shift o, immediate shift i.
 * - Iw,s: Immediate with optional shift, width w, shift s.
 * - Hs,i: Optional register shift, excluding ROR, shift s, immediate shift i.
 * - X: Check that at least one of the two registers is SP.
 *
 * - r: Source register, bits 5-9
 */

/*
 * An instruction.
 *
 * Fields:
 * - name: The instruction name.
 * - parameters: The instruction encoding instructions.
 * - base: The base code.
 */
typedef struct CymbInstruction
{
	const char* name;
	const char* parameters;
	uint32_t base;
} CymbInstruction;

// Must be stored in alphabetical order of instruction names.
const CymbInstruction aarch64Instructions[] = {
	{.name = "ret", .parameters = "r", .base = 0xd65f0000}
};
constexpr size_t aarch64InstructionCount = CYMB_LENGTH(aarch64Instructions);
constexpr size_t aarch64InstructionSize = sizeof(aarch64Instructions[0]);

const CymbInstruction x8664Instructions[] = {
	{.name = "ret", .parameters = "i", .base = 0xc2}
};
constexpr size_t x8664InstructionCount = CYMB_LENGTH(x8664Instructions);
constexpr size_t x8664InstructionSize = sizeof(x8664Instructions[0]);

/*
 * A register.
 *
 * Fields:
 * - number: The register number.
 * - isX: Flag indicating if the register is 64-bit.
 * - isZr: Flag indicating if the register is a zero register.
 * - isSp: Flag indicating if the register is a stack pointer.
 */
typedef struct CymbRegister
{
	unsigned char number: 5;

	bool isX: 1;
	bool isZr: 1;
	bool isSp: 1;
} CymbRegister;

/*
 * An immediate.
 *
 * Fields:
 * - value: The value or its two's complement if it is negative.
 * - isNegative: Flag indicating if the immediate is negative.
 */
typedef struct CymbImmediate
{
	uint64_t value;
	bool isNegative;
} CymbImmediate;

/*
 * Compare two instructions by name.
 *
 * Parameters:
 * - nameVoid: An instruction name to test.
 * - instructionVoid: An instruction.
 *
 * Returns:
 * - 0 if the names are equal.
 * - A negative value if the name comes before the instruction.
 * - A positive value if the name comes after the instruction.
 */
static int cymbCompareInstructions(const void* const nameVoid, const void* const instructionVoid)
{
	const char* const name = nameVoid;
	const CymbInstruction* const instruction = instructionVoid;

	return strcmp(name, instruction->name);
}

/*
 * Parse a register.
 *
 * Parameters:
 * - reader: A reader.
 * - parsed: The parsed register.
 * - diagnostics: A diagnostic list.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 */
static CymbResult cymbParseRegister(CymbReader* const reader, CymbRegister* const parsed, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	CymbDiagnosticInfo info = {
		.position = reader->position,
		.line = reader->line,
		.hint = {.string = reader->string}
	};

	const char characters[4] = {
		tolower((unsigned char)reader->string[0]),
		characters[0] == '\0' ? '\0' : tolower((unsigned char)reader->string[1]),
		characters[1] == '\0' ? '\0' : tolower((unsigned char)reader->string[2]),
		characters[2] == '\0' ? '\0' : reader->string[3]
	};

	unsigned char endCharacterIndex = 3;

	*parsed = (CymbRegister){};

	if(characters[0] == 'w' && characters[1] == 'z' && characters[2] == 'r')
	{
		parsed->number = 31;
		parsed->isZr = true;

		goto end;
	}

	if(characters[0] == 'x' && characters[1] == 'z' && characters[2] == 'r')
	{
		parsed->number = 31;
		parsed->isZr = true;
		parsed->isX = true;

		goto end;
	}

	if(characters[0] == 'w' && characters[1] == 's' && characters[2] == 'p')
	{
		parsed->number = 31;
		parsed->isSp = true;

		goto end;
	}

	if(characters[0] == 's' && characters[1] == 'p')
	{
		parsed->number = 31;
		parsed->isSp = true;
		parsed->isX = true;

		endCharacterIndex = 2;

		goto end;
	}

	if(characters[0] == 'l' && characters[1] == 'r')
	{
		parsed->number = 30;
		parsed->isX = true;

		endCharacterIndex = 2;

		goto end;
	}

	if(characters[0] != 'w' && characters[0] != 'x')
	{
		result = CYMB_INVALID;
		goto end;
	}
	parsed->isX = characters[0] == 'x';

	if(!isdigit(characters[1]))
	{
		result = CYMB_INVALID;
		goto end;
	}
	parsed->number = characters[1] - '0';

	const bool isDigit = isdigit(characters[2]);
	endCharacterIndex = 2 + isDigit;
	if(isDigit)
	{
		if(parsed->number == 0)
		{
			result = CYMB_INVALID;
			goto end;
		}

		parsed->number = parsed->number * 10 + characters[2] - '0';

		if(parsed->number > 30)
		{
			result = CYMB_INVALID;
			goto end;
		}
	}

	end:
	if(result == CYMB_SUCCESS && (isalnum(characters[endCharacterIndex]) || characters[endCharacterIndex] == '_'))
	{
		result = CYMB_INVALID;
	}

	while(isalnum((unsigned char)*reader->string) || *reader->string == '_')
	{
		cymbReaderPop(reader);
	}

	if(result != CYMB_SUCCESS)
	{
		info.hint.length = reader->string - info.hint.string;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_REGISTER,
			.info = info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}
	}

	return result;
}

/*
 * Parse an immediate.
 *
 * Parameters:
 * - reader: A reader.
 * - parsed: The parsed register.
 * - diagnostics: A diagnostic list.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 */
static CymbResult cymbParseImmediate(CymbReader* const reader, CymbImmediate* const parsed, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	CymbDiagnosticInfo info = {
		.position = reader->position,
		.line = reader->line,
		.hint = {reader->string, 1}
	};

	if(*reader->string != '#' && !isalnum((unsigned char)*reader->string))
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_IMMEDIATE,
			.info = info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}
	if(*reader->string == '#')
	{
		cymbReaderPop(reader);
		cymbReaderSkipSpacesInLine(reader);
	}

	parsed->isNegative = *reader->string == '-';
	if(parsed->isNegative)
	{
		cymbReaderPop(reader);
	}

	uintmax_t value;
	result = cymbParseUnsigned(reader, &value, 0, diagnostics);

	const char* const end = reader->string;

	while(isalnum((unsigned char)*reader->string) || *reader->string == '_')
	{
		cymbReaderPop(reader);
	}
	info.hint.length = reader->string - info.hint.string;

	if(end != reader->string)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_IMMEDIATE,
			.info = info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}

	if(result != CYMB_SUCCESS)
	{
		if(result == CYMB_NO_MATCH)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_IMMEDIATE,
				.info = info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}
		}

		result = result == CYMB_NO_MATCH ? CYMB_INVALID : result;
		goto end;
	}
	if(value > UINT64_MAX)
	{
		result = CYMB_INVALID;

		const CymbDiagnostic diagnostic = {
			.type = CYMB_INVALID_IMMEDIATE,
			.info = info
		};
		const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
		if(diagnosticResult != CYMB_SUCCESS)
		{
			result = diagnosticResult;
		}

		goto end;
	}
	parsed->value = value;

	if(parsed->value == 0)
	{
		parsed->isNegative = false;
	}

	if(parsed->isNegative)
	{
		if(parsed->value > (INT64_MAX + UINT64_C(1)))
		{
			result = CYMB_INVALID;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_IMMEDIATE,
				.info = info
			};
			const CymbResult diagnosticResult = cymbDiagnosticAdd(diagnostics, &diagnostic);
			if(diagnosticResult != CYMB_SUCCESS)
			{
				result = diagnosticResult;
			}

			goto end;
		}

		parsed->value = UINT64_MAX - parsed->value + 1;
	}

	end:
	return result;
}

static CymbResult cymbParseX8664Instruction(CymbReader* const reader, const CymbInstruction* const instruction, CymbSegmentedArray* const codesArray, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	char* code = cymbSegmentArrayNext(codesArray);
	if(!code)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto error;
	}
	*code = instruction->base;

	const char* parameters = instruction->parameters;
	while(*parameters != '\0')
	{
		switch(*parameters)
		{
			case 'i':
				CymbImmediate immediate;

				if(!isspace((unsigned char)*reader->string) && *reader->string != '\0')
				{
					goto error;
				}

				cymbReaderSkipSpacesInLine(reader);

				if(*reader->string == '\n' || *reader->string == '\0')
				{
					++*code;
					break;
				}

				result = cymbParseImmediate(reader, &immediate, diagnostics);
				if(result != CYMB_SUCCESS || immediate.value > UINT16_MAX)
				{
					goto error;
				}

				code = cymbSegmentArrayNext(codesArray);
				if(!code)
				{
					result = CYMB_OUT_OF_MEMORY;
					goto error;
				}
				*code = immediate.value & 0xff;

				code = cymbSegmentArrayNext(codesArray);
				if(!code)
				{
					result = CYMB_OUT_OF_MEMORY;
					goto error;
				}
				*code = immediate.value >> 8 & 0xff;

				break;
		}

		++parameters;
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS ? CYMB_INVALID : result;

	end:
	return result;
}

/*
 * Parse an instruction.
 *
 * Parameters:
 * - reader: A reader.
 * - labels: A list of labels.
 * - offset: The current offset.
 * - instruction: The instruction to test.
 * - code: The parsed code.
 * - diagnostics: A list of diagnostics.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_NO_MATCH if it does not match.
 * - CYMB_INVALID if it is invalid.
 */
static CymbResult cymbParseAarch64Instruction(CymbReader* const reader, const CymbInstruction* const instruction, CymbSegmentedArray* const codesArray, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	uint32_t* const code = cymbSegmentArrayNext(codesArray);
	if(!code)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto error;
	}
	*code = instruction->base;

	const char* parameters = instruction->parameters;
	while(*parameters != '\0')
	{
		switch(*parameters)
		{
			case 'r':
			{
				CymbRegister sourceRegister;

				if(!isspace((unsigned char)*reader->string) && *reader->string != '\0')
				{
					goto error;
				}

				cymbReaderSkipSpacesInLine(reader);

				if(*reader->string == '\n' || *reader->string == '\0')
				{
					sourceRegister.number = 30;
				}
				else
				{
					result = cymbParseRegister(reader, &sourceRegister, diagnostics);
					if(result != CYMB_SUCCESS || sourceRegister.isSp || !sourceRegister.isX)
					{
						goto error;
					}
				}

				*code |= (uint32_t)sourceRegister.number << 5;

				break;
			}
		}

		++parameters;
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS ? CYMB_INVALID : result;

	end:
	return result;
}

CymbResult cymbAssemble(const char* const string, char** const codes, size_t* const size, const CymbMachine machine, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*size = 0;
	*codes = nullptr;

	CymbReader reader;
	cymbReaderCreate(string, diagnostics->tabWidth, &reader);

	CymbSegmentedArray codesArray;
	if(machine == CYMB_X86_64)
	{
		cymbSegmentedArrayCreate(&codesArray, diagnostics->diagnostics.arena, sizeof(char), alignof(char));
	}
	else
	{
		cymbSegmentedArrayCreate(&codesArray, diagnostics->diagnostics.arena, sizeof(uint32_t), alignof(uint32_t));
	}

	while(*reader.string != '\0')
	{
		cymbReaderSkipSpaces(&reader);

		CymbDiagnosticInfo info = {
			.position = reader.position,
			.line = reader.line,
			.hint = {.string = reader.string}
		};

		char name[5];
		unsigned char nameIndex = 0;
		while(nameIndex < sizeof(name) - 1 && (isalnum((unsigned char)*reader.string) || *reader.string == '_'))
		{
			name[nameIndex] = tolower((unsigned char)*reader.string);
			++nameIndex;

			cymbReaderPop(&reader);
		}
		info.hint.length = reader.string - info.hint.string;

		if(nameIndex == sizeof(name) - 1 && (isalnum((unsigned char)*reader.string) || *reader.string == '_'))
		{
			while(isalnum((unsigned char)*reader.string) || *reader.string == '_')
			{
				cymbReaderPop(&reader);
			}
			info.hint.length = reader.string - info.hint.string;

			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNKNOWN_INSTRUCTION,
				.info = info
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);

			goto error;
		}
		name[nameIndex] = '\0';

		const CymbInstruction* instruction;
		if(machine == CYMB_X86_64)
		{
			instruction = bsearch(name, x8664Instructions, x8664InstructionCount, x8664InstructionSize, cymbCompareInstructions);
		}
		else
		{
			instruction = bsearch(name, aarch64Instructions, aarch64InstructionCount, aarch64InstructionSize, cymbCompareInstructions);
		}
		if(!instruction)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNKNOWN_INSTRUCTION,
				.info = info
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			
			goto error;
		}

		if(machine == CYMB_X86_64)
		{
			result = cymbParseX8664Instruction(&reader, instruction, &codesArray, diagnostics);
		}
		else
		{
			result = cymbParseAarch64Instruction(&reader, instruction, &codesArray, diagnostics);
		}

		if(result != CYMB_SUCCESS)
		{
			goto error;
		}

		cymbReaderSkipSpacesInLine(&reader);

		if(*reader.string != '\n' && *reader.string != '\0')
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNEXPECTED_CHARACTERS_AFTER_INSTRUCTION,
				.info = {
					.position = reader.position,
					.line = reader.line,
					.hint = {reader.string, reader.line.length - (reader.string - reader.line.string)}
				}
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			
			goto error;
		}

		while(*reader.string != '\0' && *reader.string != '\n')
		{
			cymbReaderPop(&reader);
		}
		if(*reader.string != '\0')
		{
			cymbReaderPop(&reader);
		}
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS || result == CYMB_NO_MATCH ? CYMB_INVALID : result;
	*codes = nullptr;

	end:
	if(codesArray.size > 0)
	{
		*size = codesArray.size;
		*codes = cymbArenaAllocate(diagnostics->diagnostics.arena, *size, alignof(typeof((*codes)[0])));
		if(!*codes)
		{
			return CYMB_OUT_OF_MEMORY;
		}

		cymbSegmentedArrayCopy(&codesArray, *codes);
	}

	return result;
}
