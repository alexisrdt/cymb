#include "stdio.h"

#include "string.h"

#include "fcntl.h"
#include "unistd.h"

typedef struct FILE
{
	int descriptor;
	bool open;
} FILE;

FILE files[FOPEN_MAX] = {
	{
		.descriptor = 0,
		.open = true
	},
	{
		.descriptor = 1,
		.open = true
	},
	{
		.descriptor = 2,
		.open = true
	}
};

FILE* stdin = &files[0];
FILE* stdout = &files[1];
FILE* stderr = &files[2];

FILE* fopen(const char* restrict const filename, const char* restrict mode)
{
	int flags = 0;
	constexpr mode_t fileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	switch(*mode)
	{
		case 'r':
			flags = O_RDONLY;
			break;

		case 'w':
			flags = O_WRONLY | O_CREAT | O_TRUNC;
			break;

		case 'a':
			flags = O_WRONLY | O_CREAT | O_APPEND;
			break;

		default:
			goto error;
	}

	char previous = '\0';
	for(unsigned char index = 0; index < 2; ++index)
	{
		++mode;
		if(*mode == '\0')
		{
			break;
		}

		if(*mode == previous)
		{
			goto error;
		}
		previous = *mode;

		if(*mode == 'b')
		{
			continue;
		}

		if(*mode == '+')
		{
			int mask = flags & O_RDONLY ? O_RDONLY : O_WRONLY;
			mask = ~mask;

			flags &= mask;
			flags |= O_RDWR;

			continue;
		}

		goto error;
	}
	
	++mode;
	if(*mode == 'x')
	{
		if(!(flags & O_TRUNC))
		{
			goto error;
		}

		flags |= O_EXCL;
		++mode;
	}

	if(*mode != '\0')
	{
		goto error;
	}

	for(size_t fileIndex = 0; fileIndex < FOPEN_MAX; ++fileIndex)
	{
		if(!files[fileIndex].open)
		{
			files[fileIndex].descriptor = openat(AT_FDCWD, filename, flags, fileMode);
			files[fileIndex].open = files[fileIndex].descriptor != -1;

			if(!files[fileIndex].open)
			{
				goto error;
			}

			return &files[fileIndex];
		}
	}

	error:
	return nullptr;
}

int fclose(FILE* const stream)
{
	stream->open = false;

	return close(stream->descriptor) * EOF;
}

size_t fread(void* restrict const array, const size_t elementSize, const size_t elementCount, FILE* const restrict stream)
{
	const ssize_t result = read(stream->descriptor, array, elementSize * elementCount);

	return result == -1 ? 0 : result;
}

size_t fwrite(const void* restrict const array, const size_t elementSize, const size_t elementCount, FILE* const restrict stream)
{
	const ssize_t result = write(stream->descriptor, array, elementSize * elementCount);

	return result == -1 ? 0 : result;
}

int fputs(const char* restrict const string, FILE* restrict const stream)
{
	const size_t length = strlen(string);
	if(length == 0)
	{
		return 0;
	}

	return (fwrite(string, 1, length, stream) != length) * EOF;
}

int puts(const char* const string)
{
	const int result = fputs(string, stdout);
	if(result == EOF)
	{
		return result;
	}

	const char newline = '\n';
	return (fwrite(&newline, 1, 1, stdout) != 1) * EOF;
}
