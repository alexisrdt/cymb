#ifndef CYMB_ARENA_H
#define CYMB_ARENA_H

#include <stddef.h>
#include <stdint.h>

/*
 * Maximum size of an object.
 */
constexpr size_t cymbSizeMax = PTRDIFF_MAX;

/*
 * An arena region.
 *
 * Fields:
 * - previous: The previous region.
 * - next: The next region.
 * - capacity: The capacity.
 * - size: The used size.
 * - region: The region.
 */
typedef struct CymbRegion
{
	struct CymbRegion* previous;
	struct CymbRegion* next;

	size_t capacity;
	size_t size;

	alignas(max_align_t) char region[];
} CymbRegion;

/*
 * Maximum size of an arena allocation.
 */
constexpr size_t cymbArenaSizeMax = cymbSizeMax - sizeof(CymbRegion);

/*
 * An arena.
 *
 * Fields:
 * - start: The first region.
 * - end: The last region.
 * - regionCapacity: The next region capacity.
 */
typedef struct CymbArena
{
	CymbRegion* start;
	CymbRegion* end;

	size_t regionCapacity;
} CymbArena;

/*
 * An arena save.
 *
 * Fields:
 * - region: The current last region.
 * - size: The current size of the last region.
 */
typedef struct CymbArenaSave
{
	CymbRegion* region;
	size_t size;
} CymbArenaSave;

/*
 * Create an arena.
 *
 * Parameters:
 * - arena: The arena.
 */
void cymbArenaCreate(CymbArena* arena);

/*
 * Free an arena.
 *
 * Parameters:
 * - arena: The arena.
 */
void cymbArenaFree(CymbArena* arena);

/*
 * Allocate a chunk of memory from an arena.
 *
 * Parameters:
 * - arena: The arena.
 * - size: The size of the allocation. Must be positive and at most cymbArenaSizeMax.
 * - alignment: The alignment of the allocation. Must be a power of two and at most alignof(max_align_t).
 *
 * Returns:
 * - A pointer to a chunk of memory on success.
 * - nullptr if memory allocation failed.
 */
void* cymbArenaAllocate(CymbArena* arena, size_t size, size_t alignment);

/*
 * Clear the arena.
 *
 * Parameters:
 * - arena: The arena.
 */
void cymbArenaClear(CymbArena* arena);

/*
 * Save the state of an arena.
 *
 * Parameters:
 * - arena: The arena.
 *
 * Returns:
 * - An arena save.
 */
CymbArenaSave cymbArenaSave(const CymbArena* arena);

/*
 * Restore an arena state from a save.
 *
 * Parameters:
 * - arena: The arena.
 * - save: The save.
 */
void cymbArenaRestore(CymbArena* arena, CymbArenaSave save);

/*
 * A segment.
 *
 * Fields:
 * - next: The next segment.
 * - capacity: The capacity.
 * - size: The used size.
 * - segment: The segment.
 */
typedef struct CymbSegment
{
	struct CymbSegment* next;

	size_t capacity;
	size_t size;

	void* segment;
} CymbSegment;

/*
 * A segmented array.
 *
 * Fields:
 * - arena: The arena to allocate from.
 * - start: The first segment.
 * - end: The last segment.
 * - elementSize: The element size.
 * - elementAlignment: The element alignment.
 * - size: The total array size.
 */
typedef struct CymbSegmentedArray
{
	CymbArena* arena;

	CymbSegment* start;
	CymbSegment* end;

	size_t elementSize;
	size_t elementAlignment;

	size_t size;
} CymbSegmentedArray;

/*
 * Create a segmented array.
 *
 * Parameters:
 * - array: The array.
 * - arena: The arena to allocate from.
 * - elementSize: The element size. Must be positive and at most cymbArenaSizeMax.
 * - elementAlignment: The element alignment. Must be a power of two and at most alignof(max_align_t).
 */
void cymbSegmentedArrayCreate(CymbSegmentedArray* array, CymbArena* arena, size_t elementSize, size_t elementAlignment);

/*
 * Get the next element in a segmented array.
 *
 * Parameters:
 * - array: The array.
 *
 * Returns:
 * - A pointer to the next element on success.
 * - nullptr on failure.
 */
void* cymbSegmentArrayNext(CymbSegmentedArray* array);

/*
 * Copy the array to a contiguous destination.
 *
 * Parameters:
 * - array: The array.
 * - destination: The destination.
 */
void cymbSegmentedArrayCopy(const CymbSegmentedArray* array, void* destination);

#endif
