#ifndef CYMB_ELF_H
#define CYMB_ELF_H

#include "cymb/diagnostic.h"

typedef struct CymbObjectFileData
{
	void* text;
	size_t textSize;

	const void* data;
	size_t dataSize;
	size_t dataAlignment;

	size_t bssSize;
	size_t bssAlignment;
} CymbObjectFileData;

CymbResult cymbCreateObjectFile(const char* fileName, const CymbObjectFileData* data);

CymbResult cymbLink(const char* const* fileNames, size_t count, CymbDiagnosticList* diagnostics);

#endif
