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
 * - Zs: GRP or ZR, shift s.
 * - Ss: GRP or SP, shift s.
 * - Es,o,i: Extended register, shift s, option shift o, immediate shift i.
 * - Iw,s: Immediate with optional shift, width w, shift s.
 * - Hs,i: Optional register shift, excluding ROR, shift s, immediate shift i.
 * - Rs,i: Optional register shift, including ROR, shift s, immediate shift i.
 * - X: Check that at least one of the two registers is SP.
 * - B: Bitmask immediate.
 * - L: Label or dot.
 *
 * Conditions:
 * - S: At least one register is SP.
 * - Z: First register is ZR.
 */


/*
 * An instruction.
 *
 * Fields:
 * - name: The instruction name.
 * - parameters: The instruction encoding instructions.
 * - base: The base code.
 * - mask: The code mask.
 * - preferredDisassembly: The preferred disassembly.
 * - preferredDisassemblyCondition: The condition to use the preferred disassembly.
 */
typedef struct CymbInstruction
{
	const char* name;
	const char* parameters;
	uint32_t base;
	uint32_t mask;

	const struct CymbInstruction* preferredDisassembly;
	const char* preferredDisassemblyCondition;
} CymbInstruction;

// Must be stored in alphabetical order of instruction names.
const CymbInstruction instructions[] = {
	{.name = "ABS", .parameters = "A31Z0Z5", .base = 0b0101'1010'1100'0000'0010'0000'0000'0000, .mask = 0b0111'1111'1111'1111'1111'1100'0000'0000},
	{.name = "ADC", .parameters = "A31Z0Z5Z16", .base = 0b0001'1010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1110'0000'1111'1100'0000'0000},
	{.name = "ADCS", .parameters = "A31Z0Z5Z16", .base = 0b0011'1010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1110'0000'1111'1100'0000'0000},
	{.name = "ADD", .parameters = "A31S0S5E16,13,10", .base = 0b0000'1011'0010'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1110'0000'0000'0000'0000'0000},
	{.name = "ADD", .parameters = "A31S0S5I12,10", .base = 0b0001'0001'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1000'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 17, .preferredDisassemblyCondition = "S"},
	{.name = "ADD", .parameters = "A31Z0Z5Z16H22,10", .base = 0b0000'1011'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'0010'0000'0000'0000'0000'0000},
	{.name = "ADDS", .parameters = "A31Z0S5E16,13,10", .base = 0b0010'1011'0010'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1110'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 14, .preferredDisassemblyCondition = "Z"},
	{.name = "ADDS", .parameters = "A31Z0S5I12,10", .base = 0b0011'0001'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1000'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 15, .preferredDisassemblyCondition = "Z"},
	{.name = "ADDS", .parameters = "A31Z0Z5Z16H22,10", .base = 0b0010'1011'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'0010'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 16, .preferredDisassemblyCondition = "Z"},
	{.name = "ADR", .parameters = "Z0L", .base = 0b0001'0000'0000'0000'0000'0000'0000'0000, .mask = 0b1001'1111'0000'0000'0000'0000'0000'0000},
	{.name = "AND", .parameters = "A31S0Z5B", .base = 0b0001'0010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1000'0000'0000'0000'0000'0000},
	{.name = "AND", .parameters = "A31Z0Z5Z16R22,10", .base = 0b0000'1010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'0010'0000'0000'0000'0000'0000},
	{.name = "ANDS", .parameters = "A31Z0Z5B", .base = 0b0111'0010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1000'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 18, .preferredDisassemblyCondition = "Z"},
	{.name = "ANDS", .parameters = "A31Z0Z5Z16R22,10", .base = 0b0110'1010'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'0010'0000'0000'0000'0000'0000, .preferredDisassembly = instructions + 19, .preferredDisassemblyCondition = "Z"},
	{.name = "CMN", .parameters = "A31S5E16,13,10", .base = 0b0010'1011'0010'0000'0000'0000'0001'1111, .mask = 0b0111'1111'1110'0000'0000'0000'0001'1111},
	{.name = "CMN", .parameters = "A31S5I12,10", .base = 0b0011'0001'0000'0000'0000'0000'0001'1111, .mask = 0b0111'1111'1000'0000'0000'0000'0001'1111},
	{.name = "CMN", .parameters = "A31Z5Z16H22,10", .base = 0b0010'1011'0000'0000'0000'0000'0001'1111, .mask = 0b0111'1111'0010'0000'0000'0000'0001'1111},
	{.name = "MOV", .parameters = "A31S0S5X", .base = 0b0001'0001'0000'0000'0000'0000'0000'0000, .mask = 0b0111'1111'1111'1111'1111'1100'0000'0000},
	{.name = "TST", .parameters = "A31Z5B", .base = 0b0111'0010'0000'0000'0000'0000'0001'1111, .mask = 0b0111'1111'1000'0000'0000'0000'0001'1111},
	{.name = "TST", .parameters = "A31Z5Z16R22,10", .base = 0b0110'1010'0000'0000'0000'0000'0001'1111, .mask = 0b0111'1111'0010'0000'0000'0000'0001'1111}
};
constexpr size_t instructionCount = CYMB_LENGTH(instructions);
constexpr size_t instructionSize = sizeof(instructions[0]);

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
 * A label.
 *
 * Fields:
 * - offset: The offset.
 */
typedef struct CymbLabel
{
	size_t offset;
} CymbLabel;

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
 * Compare two instrucitons by code.
 *
 * Parameters:
 * - codeVoid: A code to test.
 * - instructionVoid: An instruction.
 *
 * Returns:
 * - 1 if the code matches the instruction.
 * - 0 otherwise.
 */
static int cymbCompareCodes(const void* const codeVoid, const void* const instructionVoid)
{
	const uint32_t* const code = codeVoid;
	const CymbInstruction* const instruction = instructionVoid;

	return (*code & instruction->mask) != instruction->base;
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
		toupper((unsigned char)reader->string[0]),
		characters[0] == '\0' ? '\0' : toupper((unsigned char)reader->string[1]),
		characters[1] == '\0' ? '\0' : toupper((unsigned char)reader->string[2]),
		characters[2] == '\0' ? '\0' : reader->string[3]
	};

	unsigned char endCharacterIndex = 3;

	*parsed = (CymbRegister){};

	if(characters[0] == 'W' && characters[1] == 'Z' && characters[2] == 'R')
	{
		parsed->number = 31;
		parsed->isZr = true;

		goto end;
	}

	if(characters[0] == 'X' && characters[1] == 'Z' && characters[2] == 'R')
	{
		parsed->number = 31;
		parsed->isZr = true;
		parsed->isX = true;

		goto end;
	}

	if(characters[0] == 'W' && characters[1] == 'S' && characters[2] == 'P')
	{
		parsed->number = 31;
		parsed->isSp = true;

		goto end;
	}

	if(characters[0] == 'S' && characters[1] == 'P')
	{
		parsed->number = 31;
		parsed->isSp = true;
		parsed->isX = true;

		endCharacterIndex = 2;

		goto end;
	}

	if(characters[0] == 'L' && characters[1] == 'R')
	{
		parsed->number = 30;
		parsed->isX = true;

		endCharacterIndex = 2;

		goto end;
	}

	if(characters[0] != 'W' && characters[0] != 'X')
	{
		result = CYMB_INVALID;
		goto end;
	}
	parsed->isX = characters[0] == 'X';

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

	if(*reader->string != '#')
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
	cymbReaderPop(reader);

	cymbReaderSkipSpacesInLine(reader);

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
static CymbResult cymbParseInstruction(CymbReader* const reader, CymbMap* const labels, const size_t offset, const CymbInstruction* const instruction, uint32_t* const code, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*code = instruction->base;

	unsigned char isXOffset = 32;
	bool isX = true;
	CymbRegister registers[4];
	unsigned char registerCount = 0;

	bool firstArgument = true;
	const char* parameters = instruction->parameters;
	while(*parameters != '\0')
	{
		const char parameter = *parameters;
		++parameters;

		switch(parameter)
		{
			case 'A':
			{
				char* end;

				isXOffset = strtoul(parameters, &end, 10);
				parameters = end - 1;

				break;
			}

			case 'Z':
			case 'S':
			{
				if(firstArgument)
				{
					if(!isspace((unsigned char)*reader->string))
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_MISSING_SPACE,
							.info = {
								.position = {reader->position.line, reader->position.column - 1},
								.line = reader->line,
								.hint = {reader->string - 1, 1}
							}
						};

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}
				}
				else
				{
					cymbReaderSkipSpacesInLine(reader);

					if(*reader->string != ',')
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_MISSING_COMMA,
							.info = {
								.position = {reader->position.line, reader->position.column - 1},
								.line = reader->line,
								.hint = {reader->string - 1, 1}
							}
						};

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}
					cymbReaderPop(reader);
				}
				firstArgument = false;

				cymbReaderSkipSpacesInLine(reader);

				CymbDiagnostic diagnostic = {
					.info = {
						.position = reader->position,
						.line = reader->line,
						.hint = {.string = reader->string}
					}
				};

				if(*reader->string == '\n' || *reader->string == '\0')
				{
					diagnostic.type = CYMB_EXPECTED_REGISTER;
					--diagnostic.info.position.column;
					--diagnostic.info.hint.string;
					diagnostic.info.hint.length = 1;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				result = cymbParseRegister(reader, &registers[registerCount], diagnostics);

				diagnostic.info.hint.length = reader->string - diagnostic.info.hint.string;

				if(result != CYMB_SUCCESS)
				{
					goto error;
				}

				if(parameter == 'Z' && registers[registerCount].isSp)
				{
					diagnostic.type = CYMB_INVALID_SP;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					result = CYMB_NO_MATCH;
					goto end;
				}
				if(parameter == 'S' && registers[registerCount].isZr)
				{
					diagnostic.type = CYMB_INVALID_ZR;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					result = CYMB_NO_MATCH;
					goto end;
				}

				if(isXOffset < 32 && registerCount == 0)
				{
					isX = registers[registerCount].isX;
				}
				else if(registers[registerCount].isX != isX)
				{
					diagnostic.type = CYMB_INVALID_REGISTER_WIDTH;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				char* end;
				const unsigned char shift = strtoul(parameters, &end, 10);
				parameters = end - 1;

				*code |= (uint32_t)registers[registerCount].number << shift;

				++registerCount;

				break;
			}

			case 'E':
			{
				char* end;

				const unsigned char shift = strtoul(parameters, &end, 10);
				parameters = end + 1;

				const unsigned char optionShift = strtoul(parameters, &end, 10);
				parameters = end + 1;

				const unsigned char immediateShift = strtoul(parameters, &end, 10);
				parameters = end - 1;

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string != ',')
				{
					const CymbDiagnostic diagnostic = {
						.type = CYMB_MISSING_COMMA,
						.info = {
							.position = {reader->position.line, reader->position.column - 1},
							.line = reader->line,
							.hint = {reader->string - 1, 1}
						}
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				CymbDiagnostic diagnostic = {
					.info = {
						.position = reader->position,
						.line = reader->line,
						.hint = {.string = reader->string}
					}
				};

				if(*reader->string == '\n' || *reader->string == '\0')
				{
					result = CYMB_NO_MATCH;

					goto end;
				}

				result = cymbParseRegister(reader, &registers[registerCount], diagnostics);

				diagnostic.info.hint.length = reader->string - diagnostic.info.hint.string;

				if(result != CYMB_SUCCESS)
				{
					result = CYMB_NO_MATCH;

					goto end;
				}

				if(registers[registerCount].isSp)
				{
					diagnostic.type = CYMB_INVALID_SP;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					result = CYMB_NO_MATCH;

					goto end;
				}
				if(!isX && registers[registerCount].isX)
				{
					diagnostic.type = CYMB_INVALID_REGISTER_WIDTH;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				*code |= (uint32_t)registers[registerCount].number << shift;

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string == '\n' || *reader->string == '\0')
				{
					if(isX && !registers[registerCount].isX)
					{
						diagnostic.type = CYMB_INVALID_REGISTER_WIDTH;

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					bool hasSp = false;
					for(unsigned char registerIndex = 0; registerIndex < registerCount; ++registerIndex)
					{
						if(registers[registerIndex].isSp)
						{
							hasSp = true;
							break;
						}
					}
					if(!hasSp)
					{
						result = CYMB_NO_MATCH;
						goto end;
					}

					if(isX && registers[registerCount].isX)
					{
						*code |= (uint32_t)0b011 << optionShift;
					}

					break;
				}
				if(*reader->string != ',')
				{
					diagnostic.type = CYMB_MISSING_COMMA;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				const char characters[] = {
					toupper((unsigned char)reader->string[0]),
					characters[0] == '\0' ? '\0' : toupper((unsigned char)reader->string[1]),
					characters[1] == '\0' ? '\0' : toupper((unsigned char)reader->string[2]),
					characters[2] == '\0' ? '\0' : toupper((unsigned char)reader->string[3])
				};

				bool isLsl = false;
				if(characters[0] == 'L' && characters[1] == 'S' && characters[2] == 'L' && !(isalnum(characters[3]) || characters[3] == '_'))
				{
					if(isX && !registers[registerCount].isX)
					{
						diagnostic.type = CYMB_INVALID_REGISTER_WIDTH;

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					isLsl = true;
					*code |= ((uint32_t)0b010 + isX) << optionShift;

					cymbReaderSkip(reader, 3);
				}
				else
				{
					if(
						(characters[0] != 'U' && characters[0] != 'S') ||
						characters[1] != 'X' ||
						characters[2] != 'T' ||
						(characters[3] != 'B' && characters[3] != 'H' && characters[3] != 'W' && characters[3] != 'X')
					)
					{
						diagnostic.type = CYMB_INVALID_EXTENSION;

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					if(characters[0] == 'S')
					{
						*code |= (uint32_t)0b100 << optionShift;
					}

					unsigned char extension;
					switch(characters[3])
					{
						case 'B':
							extension = 0b00;
							break;

						case 'H':
							extension = 0b01;
							break;

						case 'W':
							extension = 0b10;
							break;

						case 'X':
							extension = 0b11;
							break;

						default:
							unreachable();
					}

					*code |= (uint32_t)extension << optionShift;

					cymbReaderSkip(reader, 4);
					if(isalnum((unsigned char)*reader->string) || *reader->string == '_')
					{
						diagnostic.type = CYMB_INVALID_EXTENSION;

						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}
				}

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string == '\n' || *reader->string == '\0')
				{
					if(isLsl)
					{
						diagnostic.type = CYMB_EXPECTED_IMMEDIATE;

						cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					break;
				}

				CymbImmediate immediate;
				result = cymbParseImmediate(reader, &immediate, diagnostics);

				if(immediate.isNegative || immediate.value > 4)
				{
					diagnostic.type = CYMB_INVALID_IMMEDIATE;

					cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				*code |= immediate.value << immediateShift;

				++registerCount;

				break;
			}

			case 'I':
			{
				char* end;

				const unsigned char immediateWidth = strtoul(parameters, &end, 10);
				parameters = end + 1;

				const unsigned char shift = strtoul(parameters, &end, 10);
				parameters = end - 1;

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string != ',')
				{
					const CymbDiagnostic diagnostic = {
						.type = CYMB_MISSING_COMMA,
						.info = {
							.position = reader->position,
							.line = reader->line,
							.hint = {reader->line.string, 1}
						}
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				if(*reader->string != '#')
				{
					result = CYMB_NO_MATCH;
					goto end;
				}

				CymbDiagnostic diagnostic = {
					.info = {
						.position = reader->position,
						.line = reader->line,
						.hint = {.string = reader->string}
					}
				};

				CymbImmediate immediate;
				result = cymbParseImmediate(reader, &immediate, diagnostics);

				diagnostic.info.hint.length = reader->string - diagnostic.info.hint.string;

				if(immediate.isNegative || immediate.value >= UINT32_C(1) << immediateWidth)
				{
					diagnostic.type = CYMB_INVALID_IMMEDIATE;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				*code |= immediate.value << shift;

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string == '\n' || *reader->string == '\0')
				{
					break;
				}
				if(*reader->string != ',')
				{
					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				const char characters[] = {
					toupper((unsigned char)reader->string[0]),
					characters[0] == '\0' ? '\0' : toupper((unsigned char)reader->string[1]),
					characters[1] == '\0' ? '\0' : toupper((unsigned char)reader->string[2])
				};
				if(characters[0] != 'L' || characters[1] != 'S' || characters[2] != 'L' || isalnum((unsigned char)reader->string[3]) || reader->string[3] == '_')
				{
					goto error;
				}

				cymbReaderSkip(reader, 3);
				cymbReaderSkipSpacesInLine(reader);

				result = cymbParseImmediate(reader, &immediate, diagnostics);
				if(result != CYMB_SUCCESS)
				{
					goto error;
				}
				if(immediate.isNegative || (immediate.value != 0 && immediate.value != 12))
				{
					goto error;
				}

				*code |= (uint32_t)(immediate.value == 12) << (shift + immediateWidth);

				break;
			}

			case 'H':
			case 'R':
			{
				char* end;

				const unsigned char shift = strtoul(parameters, &end, 10);
				parameters = end + 1;

				const unsigned char immediateShift = strtoul(parameters, &end, 10);
				parameters = end - 1;

				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string == '\n' || *reader->string == '\0')
				{
					break;
				}
				if(*reader->string != ',')
				{
					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				const char characters[] = {
					toupper((unsigned char)reader->string[0]),
					characters[0] == '\0' ? '\0' : toupper((unsigned char)reader->string[1]),
					characters[1] == '\0' ? '\0' : toupper((unsigned char)reader->string[2]),
					'\0'
				};
				unsigned char shiftType;
				if(strcmp(characters, "LSL") == 0)
				{
					shiftType = 0b00;
				}
				else if(strcmp(characters, "LSR") == 0)
				{
					shiftType = 0b01;
				}
				else if(strcmp(characters, "ASR") == 0)
				{
					shiftType = 0b10;
				}
				else if(parameter == 'R' && strcmp(characters, "ROR") == 0)
				{
					shiftType = 0b11;
				}
				else
				{
					goto error;
				}

				*code |= (uint32_t)shiftType << shift;

				cymbReaderSkip(reader, 3);
				cymbReaderSkipSpacesInLine(reader);

				CymbImmediate immediate;
				result = cymbParseImmediate(reader, &immediate, diagnostics);
				if(result != CYMB_SUCCESS)
				{
					goto error;
				}

				if(immediate.isNegative || immediate.value >= 64 || (!isX && immediate.value >= 32))
				{
					goto error;
				}

				*code |= immediate.value << immediateShift;

				break;
			}

			case 'X':
			{
				if(!registers[0].isSp && !registers[1].isSp)
				{
					const CymbDiagnostic diagnostic = {
						.type = CYMB_EXPECTED_SP
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				break;
			}

			case 'B':
			{
				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string != ',')
				{
					const CymbDiagnostic diagnostic = {
						.type = CYMB_MISSING_COMMA,
						.info = {
							.position = {reader->position.line, reader->position.column - 1},
							.line = reader->line,
							.hint = {reader->string - 1, 1}
						}
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				CymbDiagnostic diagnostic = {
					.info = {
						.position = reader->position,
						.line = reader->line,
						.hint = {.string = reader->string}
					}
				};

				CymbImmediate immediate;
				result = cymbParseImmediate(reader, &immediate, diagnostics);

				diagnostic.info.hint.length = reader->string - diagnostic.info.hint.string;

				if(result != CYMB_SUCCESS)
				{
					result = CYMB_NO_MATCH;
					goto end;
				}

				if(immediate.isNegative || immediate.value == 0 || (!isX && immediate.value > UINT32_MAX))
				{
					diagnostic.type = CYMB_INVALID_IMMEDIATE;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				if(!isX)
				{
					immediate.value |= immediate.value << 32;
				}
				if(immediate.value == UINT64_MAX)
				{
					diagnostic.type = CYMB_INVALID_IMMEDIATE;

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}

				uint64_t cleared = immediate.value & (immediate.value + 1);

				unsigned char trailingZeroes = 0;
				while(trailingZeroes < 64)
				{
					if(cleared & 1)
					{
						break;
					}

					++trailingZeroes;
					cleared >>= 1;
				}
				trailingZeroes %= 64;

				const uint64_t aligned = trailingZeroes == 0 ? immediate.value : cymbRotateRight64(immediate.value, trailingZeroes);

				uint64_t alignedCount = aligned;
				unsigned char ones = 0;
				while(alignedCount & 1)
				{
					++ones;
					alignedCount >>= 1;
				}

				alignedCount = aligned;
				unsigned char zeroes = 0;
				while(!(alignedCount & 0x8000000000000000))
				{
					++zeroes;
					alignedCount <<= 1;
				}

				const unsigned char size = ones + zeroes;
				if(cymbRotateRight64(immediate.value, size) != immediate.value)
				{
					goto error;
				}

				const unsigned char immr = (size - trailingZeroes) % size;
				const unsigned char imms = (~((size << 1) - 1) | (ones - 1)) & 0b11'1111;
				const bool N = size == 64;

				*code |= (uint32_t)imms << 10;
				*code |= (uint32_t)immr << 16;
				*code |= (uint32_t)N << 22;

				break;
			}

			case 'L':
			{
				cymbReaderSkipSpacesInLine(reader);
				if(*reader->string != ',')
				{
					const CymbDiagnostic diagnostic = {
						.type = CYMB_MISSING_COMMA,
						.info = {
							.position = {reader->position.line, reader->position.column - 1},
							.line = reader->line,
							.hint = {reader->string - 1, 1}
						}
					};

					result = cymbDiagnosticAdd(diagnostics, &diagnostic);

					goto error;
				}
				cymbReaderPop(reader);
				cymbReaderSkipSpacesInLine(reader);

				if(*reader->string == '.')
				{
					cymbReaderPop(reader);
					break;
				}

				CymbStringView label = {.string = reader->string};
				if(!isalpha((unsigned char)*label.string) && *label.string != '_')
				{
					goto error;
				}
				while(isalnum((unsigned char)*reader->string) || *reader->string == '_')
				{
					cymbReaderPop(reader);
				}
				label.length = reader->string - label.string;

				const CymbLabel* const labelData = cymbMapRead(labels, label);
				if(!labelData)
				{
					break;
				}

				const int32_t labelOffset = ((int32_t)labelData->offset - offset) * 4;

				const uint32_t lo = labelOffset & 0b11;
				*code |= lo << 29;

				const uint32_t hi = labelOffset >> 2 & 0b11'1111'1111'1111'1111;
				*code |= hi << 5;

				const uint32_t s = labelOffset < 0;
				*code |= s << 23;

				break;
			}

			default:
				unreachable();
		}

		parameters += *parameters != '\0';
	}

	cymbReaderSkipSpacesInLine(reader);

	if(*reader->string != '\n' && *reader->string != '\0')
	{
		const CymbDiagnostic diagnostic = {
			.type = CYMB_UNEXPECTED_CHARACTERS_AFTER_INSTRUCTION,
			.info = {
				.position = reader->position,
				.line = reader->line,
				.hint = {reader->string, reader->line.length - (reader->string - reader->line.string)}
			}
		};
		result = cymbDiagnosticAdd(diagnostics, &diagnostic);
		
		goto error;
	}
	if(*reader->string != '\0')
	{
		cymbReaderPop(reader);
	}

	if(isXOffset < 32 && isX)
	{
		*code |= (uint32_t)isX << isXOffset;
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS ? CYMB_INVALID : result;

	end:
	return result;
}

CymbResult cymbAssemble(const char* const string, uint32_t** const codes, size_t* const count, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	*count = 0;
	*codes = nullptr;

	CymbReader reader;
	cymbReaderCreate(string, diagnostics->tabWidth, &reader);

	CymbMap map;
	result = cymbMapCreate(&map, diagnostics->arena, 32, sizeof(CymbLabel), alignof(CymbLabel));
	if(result != CYMB_SUCCESS)
	{
		goto error;
	}

	size_t capacity = 32;
	*codes = malloc(capacity * sizeof((*codes)[0]));
	if(!codes)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto error;
	}

	CymbReader labelsReader = reader;
	size_t offset = 0;
	const char* colon = strchr(labelsReader.string, ':');
	while(colon)
	{
		while(colon > labelsReader.line.string + labelsReader.line.length)
		{
			cymbReaderSkipSpacesInLine(&labelsReader);

			if(isalpha((unsigned char)*labelsReader.string) || *labelsReader.string == '_')
			{
				++offset;
			}

			cymbReaderSkipLine(&labelsReader);
		}

		cymbReaderSkipSpaces(&labelsReader);

		const char* const label = labelsReader.string;
		bool valid = isalpha((unsigned char)*label) || *label == '_';

		CymbDiagnosticInfo info = {
			.position = labelsReader.position,
			.line = labelsReader.line,
			.hint = {.string = label}
		};

		while(isalnum((unsigned char)*labelsReader.string) || *labelsReader.string == '_')
		{
			cymbReaderPop(&labelsReader);
		}
		cymbReaderSkipSpaces(&labelsReader);

		info.hint.length = labelsReader.string - label;

		if(labelsReader.string != colon)
		{
			valid = false;

			while(isspace((unsigned char)*(colon - 1)))
			{
				--colon;
			}
			cymbReaderSkip(&labelsReader, colon - labelsReader.string);

			info.hint.length = labelsReader.string - label;
		}

		if(!valid)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_INVALID_LABEL,
				.info = info
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);

			goto error;
		}

		if(cymbMapRead(&map, info.hint) != nullptr)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_DUPLICATE_LABEL,
				.info = info
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);

			goto error;
		}

		result = cymbMapStore(&map, info.hint, &(CymbLabel){.offset = offset});
		if(result != CYMB_SUCCESS)
		{
			goto error;
		}

		cymbReaderPop(&labelsReader);

		colon = strchr(colon + 1, ':');
	}

	colon = strchr(reader.string, ':');
	offset = 0;
	while(*reader.string != '\0')
	{
		if(*count == capacity)
		{
			if(capacity >= cymbSizeMax / sizeof((*codes)[0]))
			{
				result = CYMB_OUT_OF_MEMORY;
				goto error;
			}

			const size_t newCapacity = capacity >= cymbSizeMax / sizeof((*codes)[0]) / 2 ? cymbSizeMax / sizeof((*codes)[0]) : capacity * 2;

			uint32_t* const newCodes = realloc(*codes, capacity * sizeof((*codes)[0]));
			if(!newCodes)
			{
				result = CYMB_OUT_OF_MEMORY;
				goto error;
			}

			*codes = newCodes;
			capacity = newCapacity;
		}

		cymbReaderSkipSpaces(&reader);
		while(colon && colon >= reader.line.string && colon <= reader.line.string + reader.line.length)
		{
			cymbReaderSkip(&reader, colon - reader.string + 1);
			colon = strchr(colon + 1, ':');

			cymbReaderSkipSpaces(&reader);
		}

		CymbDiagnosticInfo info = {
			.position = reader.position,
			.line = reader.line,
			.hint = {.string = reader.string}
		};

		char name[5];
		unsigned char nameIndex = 0;
		while(nameIndex < sizeof(name) - 1 && (isalnum((unsigned char)*reader.string) || *reader.string == '_'))
		{
			name[nameIndex] = toupper((unsigned char)*reader.string);
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

		const CymbInstruction* instruction = bsearch(name, instructions, instructionCount, instructionSize, cymbCompareInstructions);
		if(!instruction)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNKNOWN_INSTRUCTION,
				.info = info
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);
			
			goto error;
		}

		const CymbInstruction* lastInstruction = instruction;
		while(lastInstruction < instructions + instructionCount - 1 && strcmp(lastInstruction->name, (lastInstruction + 1)->name) == 0)
		{
			++lastInstruction;
		}
		while(instruction > instructions && strcmp(instruction->name, (instruction - 1)->name) == 0)
		{
			--instruction;
		}

		for(; instruction <= lastInstruction; ++instruction)
		{
			const CymbReader readerCopy = reader;
			const CymbDiagnosticList diagnosticsCopy = *diagnostics;
			const CymbArenaSave save = cymbArenaSave(diagnostics->arena);

			result = cymbParseInstruction(&reader, &map, offset, instruction, &(*codes)[*count], diagnostics);

			if(result == CYMB_SUCCESS || result == CYMB_INVALID)
			{
				break;
			}
			if(result != CYMB_NO_MATCH)
			{
				goto error;
			}

			if(instruction != lastInstruction)
			{
				reader = readerCopy;
				*diagnostics = diagnosticsCopy;
				if(diagnostics->end)
				{
					diagnostics->end->next = nullptr;
				}
				cymbArenaRestore(diagnostics->arena, save);
			}
		}

		if(result != CYMB_SUCCESS)
		{
			goto error;
		}

		++*count;
		++offset;
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS || result == CYMB_NO_MATCH ? CYMB_INVALID : result;
	CYMB_FREE(*codes);
	*count = 0;

	end:
	cymbMapFree(&map);
	return result;
}

/*
 * Append a format string and its arguments to a string.
 *
 * Parameters:
 * - string: The base string.
 * - capacity: The capacity of the string.
 * - format: The format string.
 * - The arguments of the format string.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_INVALID if it is invalid.
 * - CYMB_OUT_OF_MEMORY if an allocation failed.
 */
static CymbResult cymbAppend(CymbString* const string, size_t* const capacity, const char* const format, ...)
{
	CymbResult result = CYMB_SUCCESS;

	va_list lengthArguments, stringArguments;
	va_start(lengthArguments);
	va_copy(stringArguments, lengthArguments);

	const int length = vsnprintf(nullptr, 0, format, lengthArguments);
	va_end(lengthArguments);

	if(length <= 0)
	{
		result = CYMB_INVALID;
		goto end;
	}

	if(*capacity - length <= string->length)
	{
		if(*capacity == cymbSizeMax)
		{
			result = CYMB_OUT_OF_MEMORY;
			goto end;
		}

		const size_t newCapacity = *capacity >= cymbSizeMax / 2 ? cymbSizeMax : *capacity * 2;

		char* const newString = realloc(string->string, newCapacity);
		if(!newString)
		{
			result = CYMB_OUT_OF_MEMORY;
			goto end;
		}

		string->string = newString;
		*capacity = newCapacity;
	}

	if(vsnprintf(string->string + string->length, length + 1, format, stringArguments) != length)
	{
		result = CYMB_INVALID;
		goto end;
	}
	string->length += length;

	end:
	va_end(stringArguments);
	return result;
}

CymbResult cymbDisassemble(const uint32_t* const codes, const size_t count, CymbString* const string, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	string->length = 0;

	size_t stringCapacity = 256;
	string->string = malloc(stringCapacity);
	if(!string->string)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto end;
	}

	for(size_t codeIndex = 0; codeIndex < count; ++codeIndex)
	{
		const CymbInstruction* instruction = cymbFind(&codes[codeIndex], instructions, instructionCount, instructionSize, cymbCompareCodes);
		if(!instruction)
		{
			const CymbDiagnostic diagnostic = {
				.type = CYMB_UNKNOWN_INSTRUCTION
			};
			result = cymbDiagnosticAdd(diagnostics, &diagnostic);

			goto error;
		}

		if(instruction->preferredDisassembly)
		{
			const bool negate = !instruction->preferredDisassemblyCondition;
			const char condition = instruction->preferredDisassemblyCondition ? *instruction->preferredDisassemblyCondition : *instruction->preferredDisassembly->preferredDisassemblyCondition;

			bool b;
			switch(condition)
			{
				case 'S':
				{
					const unsigned char firstRegister = codes[codeIndex] & 0b1'1111;
					const unsigned char secondRegister = codes[codeIndex] >> 5 & 0b1'1111;

					b = firstRegister == 31 || secondRegister == 31;

					break;
				}

				case 'Z':
				{
					const unsigned char firstRegister = codes[codeIndex] & 0b1'1111;

					b = firstRegister == 31;

					break;
				}

				default:
					unreachable();
			}

			if(negate)
			{
				b = !b;
			}

			if(b)
			{
				instruction = instruction->preferredDisassembly;
			}
		}

		result = cymbAppend(string, &stringCapacity, instruction->name);

		const char* parameters = instruction->parameters;
		bool firstParameter = true;
		bool isX = true;
		bool hasSp = false;

		while(*parameters != '\0')
		{
			const char parameter = *parameters;
			++parameters;

			switch(parameter)
			{
				case 'A':
				{
					char* end;

					const unsigned char offset = strtoul(parameters, &end, 10);
					parameters = end - 1;

					isX = codes[codeIndex] >> offset & 0b1;

					break;
				}

				case 'Z':
				case 'S':
				{
					if(!firstParameter)
					{
						result = cymbAppend(string, &stringCapacity, ",");
						if(result != CYMB_SUCCESS)
						{
							goto error;
						}
					}
					firstParameter = false;

					result = cymbAppend(string, &stringCapacity, " ");
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					char* end;
					const unsigned char shift = strtoul(parameters, &end, 10);
					parameters = end - 1;

					const unsigned char registerNumber = codes[codeIndex] >> shift & 0b1'1111;

					if(registerNumber == 31)
					{
						if(parameter == 'Z')
						{
							result = cymbAppend(string, &stringCapacity, isX ? "XZR" : "WZR");
							if(result != CYMB_SUCCESS)
							{
								goto error;
							}
						}
						else
						{
							hasSp = true;

							result = cymbAppend(string, &stringCapacity, isX ? "SP" : "WSP");
							if(result != CYMB_SUCCESS)
							{
								goto error;
							}
						}
					}
					else
					{
						result = cymbAppend(string, &stringCapacity, "%c%hhu", isX ? 'X' : 'W', registerNumber);
						if(result != CYMB_SUCCESS)
						{
							goto error;
						}
					}

					break;
				}

				case 'E':
				{
					char* end;

					const unsigned char shift = strtoul(parameters, &end, 10);
					parameters = end + 1;

					const unsigned char optionShift = strtoul(parameters, &end, 10);
					parameters = end + 1;

					const unsigned char immediateShift = strtoul(parameters, &end, 10);
					parameters = end - 1;

					const unsigned char registerNumber = codes[codeIndex] >> shift & 0b1'1111;
					const unsigned char option = codes[codeIndex] >> optionShift & 0b111;
					const unsigned char immediate = codes[codeIndex] >> immediateShift & 0b111;

					result = cymbAppend(string, &stringCapacity, ", %c%hhu", isX && option & 0b11 ? 'X' : 'W', registerNumber);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					const char extensions[] = {'B', 'H', 'W', 'X'};

					if(hasSp && ((isX && option == 0b11) || (!isX && option == 0b10)))
					{
						if(immediate == 0)
						{
							break;
						}

						result = cymbAppend(string, &stringCapacity, ", LSL #%hhu", immediate);

						break;
					}

					result = cymbAppend(string, &stringCapacity, ", %cXT%c", option & 0b100 ? 'S' : 'U', extensions[option & 0b11]);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					if(immediate != 0)
					{
						result = cymbAppend(string, &stringCapacity, " #%hhu", immediate);
						if(result != CYMB_SUCCESS)
						{
							goto error;
						}
					}

					break;
				}

				case 'I':
				{
					char* end;

					const unsigned char immediateWidth = strtoul(parameters, &end, 10);
					parameters = end + 1;

					const unsigned char shift = strtoul(parameters, &end, 10);
					parameters = end - 1;

					const uint32_t immediate = codes[codeIndex] >> shift & ((UINT32_C(1) << immediateWidth) - 1);
					result = cymbAppend(string, &stringCapacity, ", #0x%"PRIX32, immediate);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					const bool hasShift = codes[codeIndex] >> (shift + immediateWidth) & 1;
					if(hasShift)
					{
						result = cymbAppend(string, &stringCapacity, ", LSL #12");
						if(result != CYMB_SUCCESS)
						{
							goto error;
						}
					}

					break;
				}

				case 'H':
				case 'R':
				{
					char* end;

					const unsigned char shift = strtoul(parameters, &end, 10);
					parameters = end + 1;

					const unsigned char immediateShift = strtoul(parameters, &end, 10);
					parameters = end - 1;

					const unsigned char shiftType = codes[codeIndex] >> shift & 0b11;
					const unsigned char immediate = codes[codeIndex] >> immediateShift & 0b11'1111;

					if(shiftType == 0 && immediate == 0)
					{
						break;
					}
					if(!isX && immediate >= 32)
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_UNKNOWN_INSTRUCTION
						};
						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					result = cymbAppend(string, &stringCapacity, ", ");
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}
					const char* shiftTypeString;
					switch(shiftType)
					{
						case 0b00:
							shiftTypeString = "LSL";
							break;

						case 0b01:
							shiftTypeString = "LSR";
							break;

						case 0b10:
							shiftTypeString = "ASR";
							break;

						case 0b11:
							if(parameter != 'R')
							{
								const CymbDiagnostic diagnostic = {
									.type = CYMB_UNKNOWN_INSTRUCTION
								};
								result = cymbDiagnosticAdd(diagnostics, &diagnostic);

								goto error;
							}

							shiftTypeString = "ROR";
							break;

						default:
							unreachable();
					}
					result = cymbAppend(string, &stringCapacity, shiftTypeString);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					result = cymbAppend(string, &stringCapacity, " #%hhu", immediate);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					break;
				}

				case 'X':
				{
					break;
				}

				case 'B':
				{
					const unsigned char imms = codes[codeIndex] >> 10 & 0b11'1111;
					const unsigned char immr = codes[codeIndex] >> 16 & 0b11'1111;
					const bool N = codes[codeIndex] >> 22 & 0b1;

					if(!isX && N)
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_UNKNOWN_INSTRUCTION
						};
						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}
					if(!N && (imms > 0b11'1100))
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_UNKNOWN_INSTRUCTION
						};
						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					const uint64_t bases[] = {
						0x5555555555555555,
						0x1111111111111111,
						0x0101010101010101,
						0x0001000100010001,
						0x0000000100000001,
						0x0000000000000001
					};

					unsigned char immsCount = imms;
					unsigned char size = N ? 6 : 5;
					while(!N && immsCount & 0b10'0000)
					{
						immsCount <<= 1;
						--size;
					}

					const unsigned char ones = (imms & ((1 << size) - 1)) + 1;
					const unsigned char pattern = (1 << ones) - 1;

					if(immr >= (1 << size))
					{
						const CymbDiagnostic diagnostic = {
							.type = CYMB_UNKNOWN_INSTRUCTION
						};
						result = cymbDiagnosticAdd(diagnostics, &diagnostic);

						goto error;
					}

					const uint64_t rotated = cymbRotateRight64(bases[size - 1] * pattern, immr);
					result = cymbAppend(string, &stringCapacity, ", #0x%"PRIX64, rotated);
					if(result != CYMB_SUCCESS)
					{
						goto error;
					}

					break;
				}

				case 'L':
				{
					const uint32_t lo = codes[codeIndex] >> 29 & 0b11;
					const uint32_t hi = codes[codeIndex] >> 5 & 0b11'1111'1111'1111'1111;
					const uint32_t s = codes[codeIndex] >> 23 & 0b1;

					int32_t offset = hi << 2 | lo;
					if(s)
					{
						offset |= 0b1111'1111'1111 << 20;
					}

					const uint32_t o = codeIndex * 4 + offset;
					cymbAppend(string, &stringCapacity, ", 0x%"PRIX32, o);

					break;
				}

				default:
					unreachable();
			}

			parameters += *parameters != '\0';
		}

		result = cymbAppend(string, &stringCapacity, "\n");
		if(result != CYMB_SUCCESS)
		{
			goto error;
		}
	}

	goto end;

	error:
	result = result == CYMB_SUCCESS ? CYMB_INVALID : result;
	CYMB_FREE(string->string);
	string->length = 0;

	end:
	return result;
}
