#ifndef CYMB_MEMORY_H
#define CYMB_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#include "cymb/arena.h"
#include "cymb/result.h"

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

#endif
