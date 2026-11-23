#include "cymb/memory.h"

#include <stdlib.h>
#include <string.h>

#undef cymbFind
void* cymbFind(const void* const valueVoid, const void* const arrayVoid, const size_t count, const size_t size, const CymbCompare compare)
{
	for(const char* element = arrayVoid, * const end = element + count * size; element < end; element += size)
	{
		if(compare(valueVoid, element) == 0)
		{
			return (void*)element;
		}
	}

	return nullptr;
}

uint32_t cymbRotateLeft32(const uint32_t value, const unsigned char rotation)
{
	return (value << rotation) | (value >> (32 - rotation));
}

uint64_t cymbRotateRight64(const uint64_t value, const unsigned char rotation)
{
	return (value >> rotation) | (value << (64 - rotation));
}

/*
 * Perform the scramble step of Murmur3 hash.
 *
 * Parameters:
 * - chunk: A chunk of bytes.
 *
 * Returns:
 * - The scrambled chunk.
 */
static uint32_t cymbMurmur3Scramble(uint32_t chunk)
{
	chunk *= 0xcc9e2d51;
	chunk = cymbRotateLeft32(chunk, 15);
	chunk *= 0x1b873593;

	return chunk;
}

uint32_t cymbMurmur3(const unsigned char* string, const size_t length)
{
	uint32_t hash = 0;
	uint32_t chunk;

	for(size_t blockIndex = length >> 2; blockIndex > 0; --blockIndex)
	{
		memcpy(&chunk, string, sizeof(uint32_t));
		string += sizeof(uint32_t);

		hash ^= cymbMurmur3Scramble(chunk);
		hash = cymbRotateLeft32(hash, 13);
		hash = hash * 5 + 0xe6546b64;
	}

	chunk = 0;
	for(size_t characterIndex = length & 3; characterIndex > 0; --characterIndex)
	{
		chunk <<= 8;
		chunk |= string[characterIndex - 1];
	}

	hash ^= cymbMurmur3Scramble(chunk);

	hash ^= length;
	hash ^= hash >> 16;
	hash *= 0x85ebca6b;
	hash ^= hash >> 13;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;

	return hash;
}
