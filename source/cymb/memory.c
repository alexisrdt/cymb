#include "cymb/memory.h"

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
