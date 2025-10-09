#ifndef CYMB_MEMORY_H
#define CYMB_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include "cymb/result.h"

/*
 * Get the minimum of two values.
 *
 * Parameters:
 * - first: The first value.
 * - second: The second value.
 *
 * Returns:
 * - The minimum of the two values.
 */
#define CYMB_MIN(first, second) \
((first) <= (second) ? (first) : (second))

/*
 * Get the maximum of two values.
 *
 * Parameters:
 * - first: The first value.
 * - second: The second value.
 *
 * Returns:
 * - The maximum of the two values.
 */
#define CYMB_MAX(first, second) \
((first) >= (second) ? (first) : (second))

/*
 * Maximum size of an object.
 */
constexpr size_t cymbSizeMax = CYMB_MIN(PTRDIFF_MAX, SIZE_MAX);

/*
 * Get the number of elements in an array.
 *
 * Parameters:
 * - array: An array.
 *
 * Returns:
 * - The number of elements in the array.
 */
#define CYMB_LENGTH(array) \
(sizeof(array) / sizeof((array)[0]))

/*
 * Free a pointer and set it to null to avoid dangling pointers.
 *
 * Parameters:
 * - pointer: A pointer.
 */
#define CYMB_FREE(pointer) \
do \
{ \
	free(pointer); \
	(pointer) = nullptr; \
} while(false);

/*
 * Comparison function.
 *
 * Parameters:
 * - firstVoid: The first value to compare.
 * - secondVoid: The second value to compare.
 *
 * Returns:
 * - 0 if the values are equal.
 * - A negative value if the first value is less than the second value.
 * - A positive value if the first value is greater than the second value.
 */
typedef int (*CymbCompare)(const void* firstVoid, const void* secondVoid);

/*
 * Find a value in an array by exploring it linearly.
 *
 * This generic function preserves const-qualification of array elements.
 * It also does not modify the array elements, even if they are not constant.
 *
 * The behavior is undefined if the passed array is not of pointer or array type or if it is nullptr.
 *
 * Parameters:
 * - valueVoid: A pointer to the value to find.
 * - arrayVoid: The array to search.
 * - count: The number of elements in the array.
 * - size: The size of each element in the array.
 * - compare: The comparison function.
 *
 * Returns:
 * - A pointer to the value in the array if it is found.
 * - nullptr if the value is not found.
 */
void* cymbFind(const void* valueVoid, const void* arrayVoid, size_t count, size_t size, CymbCompare compare);

#define cymbFind(valueVoid, arrayVoid, count, size, compare) _Generic( \
	true ? (arrayVoid) : (void*)nullptr, \
	const void*: (const void*)cymbFind((valueVoid), (arrayVoid), (count), (size), (compare)), \
	default: cymbFind((valueVoid), (arrayVoid), (count), (size), (compare)) \
)

/*
 * A string.
 *
 * Fields:
 * - string: A string.
 * - length: The length of the string.
 */
typedef struct CymbString
{
	char* string;
	size_t length;
} CymbString;

/*
 * A constant string.
 *
 * Fields:
 * - string: A string.
 * - length: The length of the string.
 */
typedef struct CymbConstString
{
	const char* string;
	size_t length;
} CymbConstString;

/*
 * A string view.
 * Alias of CymbConstString.
 */
typedef CymbConstString CymbStringView;

/*
 * Create a CymbString from a string literal.
 *
 * Parameters:
 * - literal: A string literal.
 *
 * Returns:
 * - A CymbString.
 */
#define CYMB_STRING(literal) \
{.string = literal, .length = sizeof(literal) - 1}

/*
 * An arena region.
 *
 * Fields:
 * - next: The next region.
 * - size: The used size.
 * - capacity: The capacity.
 * - data: The data.
 */
typedef struct CymbRegion
{
	struct CymbRegion* next;

	size_t size;
	size_t capacity;

	alignas(max_align_t) char data[];
} CymbRegion;

/*
 * An arena.
 *
 * Fields:
 * - start: The first region.
 * - end: The last region.
 */
typedef struct CymbArena
{
	CymbRegion* start;
	CymbRegion* end;
} CymbArena;

/*
 * An arena save.
 *
 * Fields:
 * - region: The current last region.
 * - size: The current size of this region.
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
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if memory allocation failed.
 */
CymbResult cymbArenaCreate(CymbArena* arena);

/*
 * Free an arena.
 *
 * Parameters:
 * - arena: The arena.
 */
void cymbArenaFree(CymbArena* arena);

/*
 * Get a chunk of memory from an arena.
 *
 * Parameters:
 * - arena: The arena.
 * - size: The size of the allocation.
 * - alignment: The alignment of the allocation.
 *
 * Returns:
 * - A pointer to a chunk of memory on success.
 * - nullptr if memory allocation failed.
 */
void* cymbArenaGet(CymbArena* arena, size_t size, size_t alignment);

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
CymbArenaSave cymbArenaSave(CymbArena* arena);

/*
 * Restore an arena state from a save.
 *
 * Parameters:
 * - arena: The arena.
 * - save: The save.
 */
void cymbArenaRestore(CymbArena* arena, CymbArenaSave save);

/*
 * Rotate a 32-bit unsigned integer left.
 *
 * Parameters:
 * - value: The value to rotate.
 * - rotation: The amount by which to rotate, not 0 and less than 32.
 *
 * Returns:
 * - The rotated value.
 */
uint32_t cymbRotateLeft32(uint32_t value, unsigned char rotation);

/*
 * Rotate a 64-bit unsigned integer right.
 *
 * Parameters:
 * - value: The value to rotate.
 * - rotation: The amount by which to rotate, not 0 and less than 64.
 *
 * Returns:
 * - The rotated value.
 */
uint64_t cymbRotateRight64(uint64_t value, unsigned char rotation);

/*
 * Compute the Murmur3 hash of a string.
 *
 * Parameters:
 * - string: The string.
 * - length: The length of the string.
 *
 * Returns:
 * - The Murmur3 hash of the provided string.
 */
uint32_t cymbMurmur3(const unsigned char* string, const size_t length);

/*
 * A map pair.
 *
 * Fields:
 * - key: The key.
 * - element: The element.
 * - next: The next pair in the bin.
 */
typedef struct CymbMapPair
{
	CymbStringView key;

	void* element;

	struct CymbMapPair* next;
} CymbMapPair;

/*
 * A map.
 *
 * Fields:
 * - binCount: The number of bins.
 * - elementSize: The size of each element.
 * - elementAlignment: The alignment of each element.
 * - arena: The arena used for allocations.
 * - bins: The bins array.
 * - binElements: The first elements for each bin.
 */
typedef struct CymbMap
{
	size_t binCount;
	size_t elementSize;
	size_t elementAlignment;

	CymbArena* arena;
	CymbMapPair* bins;
	void* binElements;
} CymbMap;

/*
 * Create a map.
 *
 * Parameters:
 * - map: The map.
 * - arena: The arena used for allocations.
 * - binCount: The number of bins.
 * - elementSize: The size of each element.
 * - elementAlignment: The alignment of each element.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if allocation failed.
 */
CymbResult cymbMapCreate(CymbMap* map, CymbArena* arena, size_t binCount, size_t elementSize, size_t elementAlignment);

/*
 * Free a map.
 *
 * Parameters:
 * - map: The map.
 */
void cymbMapFree(CymbMap* map);

/*
 * Store an element in a map.
 *
 * Parameters:
 * - map: The map.
 * - key: The key of the element.
 * - element: The element.
 *
 * Returns:
 * - CYMB_SUCCESS on success.
 * - CYMB_OUT_OF_MEMORY if allocation failed.
 */
CymbResult cymbMapStore(CymbMap* map, CymbStringView key, const void* element);

/*
 * Read a value from a map.
 *
 * Parameters:
 * - map: The map.
 * - key: The key of the element.
 *
 * Returns:
 * - A pointer to the element if it was found.
 * - nullptr if the element was not found.
 */
void* cymbMapRead(const CymbMap* map, CymbStringView key);

#endif
