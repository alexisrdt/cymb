#include "cymb/version.h"

#include <stdint.h>
#include <stdio.h>

void cymbPrintVersion(void)
{
	printf("Cymb %ju.%ju.%ju\n", (uintmax_t)CYMB_VERSION_MAJOR, (uintmax_t)CYMB_VERSION_MINOR, (uintmax_t)CYMB_VERSION_PATCH);
	puts("Copyright (C) 2025 Alexis Robardet\nMIT license");
}

void cymbPrintHelp(void)
{
	puts
	(
		"Usage: cymb [options] input-files...\n"
		"Options:\n"
		"  -g --debug                  Compile in debug.\n"
		"  -h --help                   Show this help information.\n"
		"  -o --output=<output-file>   Set the output file.\n"
		"     --standard=<standard>    Set the C standard.\n"
		"     --tab-width=<tab-width>  Set the tab width for diagnostics.\n"
		"  -v --version                Show the version information."
	);
}
