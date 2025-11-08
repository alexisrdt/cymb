#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
	const char* const string =
		"Hello, this is a simple test string.\n"
		"I am trying to get written to a file.\n"
		"Have a nice day!"
	;

	FILE* file = fopen("./libc_demo.txt", "w");
	if(!file)
	{
		return 1;
	}

	const size_t length = strlen(string);
	if(fwrite(string, 1, length, file) != length)
	{
		return 1;
	}

	if(fclose(file) != 0)
	{
		return 1;
	}

	puts("String written correctly!");

	for(int i = 0; i < argc; ++i)
	{
		puts(argv[i]);
	}

	file = fopen("./libc_demo.txt", "r");
	if(!file)
	{
		return 1;
	}

	char read[128];
	if(fread(read, 1, length, file) != length)
	{
		return 1;
	}
	read[length] = '\0';

	if(fclose(file) != 0)
	{
		return 1;
	}

	puts(read);

	return 0;
}
