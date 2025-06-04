#ifndef CYMB_MEMORY_H
#define CYMB_MEMORY_H

#include <stdint.h>
#include <stdlib.h>

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

// Maximum size of an object.
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
 * - A pointer to the value in the array.
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

#endif
