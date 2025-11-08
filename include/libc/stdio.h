#ifndef STDIO_H
#define STDIO_H

#include "size_t.h"

#define EOF -1
#define FOPEN_MAX 8

typedef struct FILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

FILE* fopen(const char* restrict filename, const char* restrict mode);
int fclose(FILE* stream);

size_t fread(void* restrict array, size_t elementSize, size_t elementCount, FILE* restrict stream);
size_t fwrite(const void* restrict array, size_t elementSize, size_t elementCount, FILE* restrict stream);

int fputs(const char* restrict string, FILE* restrict stream);
int puts(const char* string);

#endif
