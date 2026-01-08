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

constexpr size_t cymbRegionSize = 0x4000;

void cymbArenaCreate(CymbArena* const arena)
{
	*arena = (CymbArena){};
}

void cymbArenaFree(CymbArena* const arena)
{
	CymbRegion* region = arena->start;
	while(region)
	{
		CymbRegion* const next = region->next;

		free(region);

		region = next;
	}

	*arena = (CymbArena){};
}

void* cymbArenaAllocate(CymbArena* const arena, const size_t size, const size_t alignment)
{
	CymbRegion* region = arena->end;
	while(region)
	{
		arena->end = region;

		size_t offset = region->size;

		const size_t alignmentOffset = offset % alignment == 0 ? 0 : alignment - offset % alignment;
		if(offset > cymbSizeMax - alignmentOffset)
		{
			goto next;
		}

		offset += alignmentOffset;

		if(size <= region->capacity && offset <= region->capacity - size)
		{
			void* const result = region->data + offset;

			region->size = offset + size;

			return result;
		}

		next:
		region = region->next;
	}

	const size_t capacity = CYMB_MAX(size, cymbRegionSize);

	region = malloc(sizeof(*region) + capacity);
	if(!region)
	{
		return nullptr;
	}

	*region = (CymbRegion){
		.size = size,
		.capacity = capacity
	};

	if(arena->end)
	{
		arena->end->next = region;
	}
	else
	{
		arena->start = region;
	}
	arena->end = region;

	return region->data;
}

void cymbArenaClear(CymbArena* arena)
{
	CymbRegion* region = arena->start;
	while(region)
	{
		region->size = 0;
		region = region->next;
	}

	arena->end = arena->start;
}

CymbArenaSave cymbArenaSave(CymbArena* const arena)
{
	return (CymbArenaSave){
		.region = arena->end,
		.size = arena->end ? arena->end->size : 0
	};
}

void cymbArenaRestore(CymbArena* const arena, const CymbArenaSave save)
{
	if(save.region)
	{
		save.region->size = save.size;
	}
	else
	{
		arena->start = save.region;
	}
	arena->end = save.region;

	CymbRegion* region = save.region ? save.region->next : nullptr;
	while(region)
	{
		region->size = 0;
		region = region->next;
	}
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
	chunk *= 0xCC9E2D51;
	chunk = cymbRotateLeft32(chunk, 15);
	chunk *= 0x1B873593;

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
		hash = hash * 5 + 0xE6546B64;
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
	hash *= 0x85EBCA6B;
	hash ^= hash >> 13;
	hash *= 0xC2B2AE35;
	hash ^= hash >> 16;

	return hash;
}

CymbResult cymbMapCreate(CymbMap* const map, CymbArena* const arena, const size_t binCount, const size_t elementSize, const size_t elementAlignment)
{
	CymbResult result = CYMB_SUCCESS;

	*map = (CymbMap){
		.binCount = binCount,
		.elementSize = elementSize,
		.elementAlignment = elementAlignment,
		.arena = arena
	};

	map->bins = cymbArenaAllocate(map->arena, binCount * sizeof(map->bins[0]), alignof(typeof(map->bins[0])));
	if(!map->bins)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto error;
	}
	for(size_t bin = 0; bin < binCount; ++bin)
	{
		map->bins[bin].key.string = nullptr;
	}

	map->binElements = cymbArenaAllocate(map->arena, binCount * elementSize, elementAlignment);
	if(!map->binElements)
	{
		result = CYMB_OUT_OF_MEMORY;
		goto error;
	}

	goto end;

	error:
	cymbMapFree(map);

	end:
	return result;
}

void cymbMapFree(CymbMap* const map)
{
	*map = (CymbMap){};
}

/*
 * Compare two keys.
 *
 * Parameters:
 * - pairKey: The key of a pair in a map.
 * - key: The key to find in the map.
 *
 * Returns:
 * - true if the keys are identical.
 * - false otherwise.
 */
static bool cymbMapCompareKeys(const CymbStringView pairKey, const CymbConstString key)
{
	return pairKey.length == key.length && memcmp(pairKey.string, key.string, key.length) == 0;
}

/*
 * Get the map bin for a key.
 *
 * Parameters:
 * - map: The map.
 * - key: The key.
 *
 * Returns:
 * - The bin index.
 */
static size_t cymbMapBin(const CymbMap* const map, const CymbConstString key)
{
	return cymbMurmur3((const unsigned char*)key.string, key.length) % map->binCount;
}

CymbResult cymbMapStore(CymbMap* const map, const CymbStringView key, const void* const element)
{
	const size_t bin = cymbMapBin(map, key);

	CymbMapPair* pair = &map->bins[bin];
	if(!pair->key.string)
	{
		pair->key = key;

		pair->element = (char*)map->binElements + bin * map->elementSize;
		memcpy(pair->element, element, map->elementSize);

		pair->next = nullptr;

		return CYMB_SUCCESS;
	}

	if(cymbMapCompareKeys(pair->key, key))
	{
		memcpy(pair->element, element, map->elementSize);

		return CYMB_SUCCESS;
	}

	while(pair->next)
	{
		pair = pair->next;

		if(cymbMapCompareKeys(pair->key, key))
		{
			memcpy(pair->element, element, map->elementSize);

			return CYMB_SUCCESS;
		}
	}
	
	CymbMapPair* const next = cymbArenaAllocate(map->arena, sizeof(*next), alignof(typeof(*next)));
	if(!next)
	{
		return CYMB_OUT_OF_MEMORY;
	}

	next->next = nullptr;

	next->key = key;

	next->element = cymbArenaAllocate(map->arena, map->elementSize, map->elementAlignment);
	memcpy(next->element, element, map->elementSize);

	pair->next = next;

	return CYMB_SUCCESS;
}

void* cymbMapRead(const CymbMap* const map, const CymbStringView key)
{
	const size_t bin = cymbMapBin(map, key);

	const CymbMapPair* pair = &map->bins[bin];
	if(!pair->key.string)
	{
		return nullptr;
	}

	while(pair)
	{
		if(cymbMapCompareKeys(pair->key, key))
		{
			return pair->element;
		}

		pair = pair->next;
	}

	return nullptr;
}
