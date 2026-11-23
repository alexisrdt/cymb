#include "cymb/arena.h"

#include <stdlib.h>
#include <string.h>

constexpr size_t cymbRegionCapacityMin = 0x1000;
constexpr size_t cymbRegionCapacityMax = 0x100000;

void cymbArenaCreate(CymbArena* const arena)
{
	*arena = (CymbArena){
		.regionCapacity = cymbRegionCapacityMin
	};
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
}

/*
 * Create a new region for an arena.
 *
 * Parameters:
 * - arena: The arena.
 * - region: The region to replace. If null, the new region is added at the end of the list.
 * - size: The size of the initial allocation.
 *
 * Returns:
 * - The beginning of the region on success.
 * - nullptr on failure.
 */
static void* cymbRegionCreate(CymbArena* const arena, CymbRegion* const region, const size_t size)
{
	const size_t capacity = size > arena->regionCapacity ? size : arena->regionCapacity;

	CymbRegion* const newRegion = malloc(sizeof(*newRegion) + capacity);
	if(!newRegion)
	{
		return nullptr;
	}

	if(capacity == arena->regionCapacity)
	{
		arena->regionCapacity = arena->regionCapacity == cymbRegionCapacityMax ? cymbRegionCapacityMax : capacity * 2;
	}

	*newRegion = (CymbRegion){
		.previous = region ? region->previous : arena->end,
		.next = region ? region->next : nullptr,
		.capacity = capacity,
		.size = size
	};

	if(newRegion->previous)
	{
		newRegion->previous->next = newRegion;
	}
	else
	{
		arena->start = newRegion;
	}

	if(newRegion->next)
	{
		newRegion->next->previous = newRegion;
	}

	free(region);

	arena->end = newRegion;

	return newRegion->region;
}

void* cymbArenaAllocate(CymbArena* const arena, const size_t size, const size_t alignment)
{
	CymbRegion* region = arena->end;
	while(region)
	{
		const size_t alignmentOffset = -region->size & (alignment - 1);
		const size_t alignedOffset = region->size + alignmentOffset;
		const size_t newSize = size + alignedOffset;

		if(newSize <= region->capacity)
		{
			arena->end = region;
			region->size = newSize;
			return region->region + alignedOffset;
		}

		if(region->size == 0)
		{
			break;
		}

		region = region->next;
		if(region)
		{
			region->size = 0;
		}
	}

	return cymbRegionCreate(arena, region, size);
}

void cymbArenaClear(CymbArena* const arena)
{
	if(arena->start)
	{
		arena->start->size = 0;
	}

	arena->end = arena->start;
}

CymbArenaSave cymbArenaSave(const CymbArena* const arena)
{
	return (CymbArenaSave){
		.region = arena->end,
		.size = arena->end ? arena->end->size : 0
	};
}

void cymbArenaRestore(CymbArena* const arena, const CymbArenaSave save)
{
	arena->end = save.region ? save.region : arena->start;

	if(arena->end)
	{
		arena->end->size = save.size;
	}
}

void cymbSegmentedArrayCreate(CymbSegmentedArray* const array, CymbArena* const arena, const size_t elementSize, const size_t elementAlignment)
{
	*array = (CymbSegmentedArray){
		.arena = arena,
		.elementSize = elementSize,
		.elementAlignment = elementAlignment
	};
}

void* cymbSegmentArrayNext(CymbSegmentedArray* const array)
{
	CymbSegment* segment = array->end;
	if(!segment || segment->size == segment->capacity)
	{
		const CymbArenaSave save = cymbArenaSave(array->arena);

		segment = cymbArenaAllocate(array->arena, sizeof(*segment), alignof(typeof(*segment)));
		if(!segment)
		{
			return nullptr;
		}

		size_t capacity;
		if(array->end && array->end->capacity <= cymbRegionCapacityMax / 2)
		{
			capacity = array->end->capacity * 2;
		}
		else
		{
			capacity = (array->end ? cymbRegionCapacityMax : cymbRegionCapacityMin) / array->elementSize;
			capacity += capacity == 0;
			capacity *= array->elementSize;
		}

		*segment = (CymbSegment){
			.capacity = capacity
		};

		segment->segment = cymbArenaAllocate(array->arena, segment->capacity, array->elementAlignment);
		if(!segment->segment)
		{
			cymbArenaRestore(array->arena, save);
			return nullptr;
		}

		if(array->end)
		{
			array->end->next = segment;
		}
		else
		{
			array->start = segment;
		}
		array->end = segment;
	}

	void* const element = (char*)segment->segment + segment->size;

	segment->size += array->elementSize;
	array->size += array->elementSize;

	return element;
}

void cymbSegmentedArrayCopy(const CymbSegmentedArray* const array, void* const destination)
{
	char* destinationChar = destination;

	CymbSegment* segment = array->start;
	while(segment)
	{
		memcpy(destinationChar, segment->segment, segment->size);
		destinationChar += segment->size;

		segment = segment->next;
	}
}
