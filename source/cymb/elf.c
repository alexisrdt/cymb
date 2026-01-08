#include "cymb/elf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libc/elf.h"

typedef struct CymbElfFile
{
	unsigned char* bytes;
	size_t size;

	Elf64_Shdr* sections;
	Elf64_Xword sectionCount;
} CymbElfFile;

CymbResult cymbCreateObjectFile(const char* const fileName, const CymbObjectFileData* const data)
{
	CymbResult result = CYMB_SUCCESS;

	size_t size = sizeof(Elf64_Ehdr);

	size += data->textSize;

	if(data->dataSize > 0 && size % data->dataAlignment != 0)
	{
		size += data->dataAlignment - size % data->dataAlignment;
	}
	size += data->dataSize;

	if(data->bssSize > 0 && size % data->bssAlignment != 0)
	{
		size += data->bssAlignment - size % data->bssAlignment;
	}

	Elf64_Half sectionCount = 2;

	size += 11;
	if(data->textSize > 0)
	{
		++sectionCount;
		size += 6;
	}
	if(data->dataSize > 0)
	{
		++sectionCount;
		size += 6;
	}
	if(data->bssSize > 0)
	{
		++sectionCount;
		size += 5;
	}

	if(size % alignof(Elf64_Shdr) != 0)
	{
		size += alignof(Elf64_Shdr) - size % alignof(Elf64_Shdr);
	}

	const Elf64_Off sectionHeadersOffset = size;

	size += 2 * sizeof(Elf64_Shdr);
	size += data->textSize > 0 ? sizeof(Elf64_Shdr) : 0;
	size += data->dataSize > 0 ? sizeof(Elf64_Shdr) : 0;
	size += data->bssSize > 0 ? sizeof(Elf64_Shdr) : 0;

	unsigned char* bytes = malloc(size);
	if(!bytes)
	{
		return CYMB_OUT_OF_MEMORY;
	}
	unsigned char* const bytesStart = bytes;

	*(Elf64_Ehdr*)bytes = (Elf64_Ehdr){
		.e_ident[EI_MAG0] = ELFMAG0,
		.e_ident[EI_MAG1] = ELFMAG1,
		.e_ident[EI_MAG2] = ELFMAG2,
		.e_ident[EI_MAG3] = ELFMAG3,
		.e_ident[EI_CLASS] = ELFCLASS64,
		.e_ident[EI_DATA] = ELFDATA2LSB,
		.e_ident[EI_VERSION] = EV_CURRENT,
		.e_type = ET_REL,
		.e_machine = EM_AARCH64,
		.e_version = EV_CURRENT,
		.e_shoff = sectionHeadersOffset,
		.e_ehsize = sizeof(Elf64_Ehdr),
		.e_shentsize = sizeof(Elf64_Shdr),
		.e_shnum = sectionCount,
		.e_shstrndx = sectionCount - 1
	};
	bytes += sizeof(Elf64_Ehdr);

	Elf64_Off textOffset = 0;
	if(data->textSize > 0)
	{
		textOffset = bytes - bytesStart;
		memcpy(bytes, data->text, data->textSize);
		bytes += data->textSize;
	}

	Elf64_Off dataOffset = 0;
	if(data->dataSize > 0)
	{
		if((bytes - bytesStart) % data->dataAlignment != 0)
		{
			bytes += data->dataAlignment - (bytes - bytesStart) % data->dataAlignment;
		}

		dataOffset = bytes - bytesStart;
		memcpy(bytes, data->data, data->dataSize);
		bytes += data->dataSize;
	}

	Elf64_Off bssOffset = 0;
	if(data->bssSize > 0)
	{
		if((bytes - bytesStart) % data->bssAlignment != 0)
		{
			bytes += data->bssAlignment - (bytes - bytesStart) % data->bssAlignment;
		}

		bssOffset = bytes - bytesStart;
	}

	const unsigned char* const namesStart = bytes;
	const Elf64_Off namesOffset = namesStart - bytesStart;

	*bytes = '\0';
	++bytes;
	Elf64_Word textNameOffset = 0;
	if(data->textSize > 0)
	{
		textNameOffset = bytes - namesStart;
		memcpy(bytes, ".text", 6);
		bytes += 6;
	}
	Elf64_Word dataNameOffset = 0;
	if(data->dataSize > 0)
	{
		dataNameOffset = bytes - namesStart;
		memcpy(bytes, ".data", 6);
		bytes += 6;
	}
	Elf64_Word bssNameOffset = 0;
	if(data->bssSize > 0)
	{
		bssNameOffset = bytes - namesStart;
		memcpy(bytes, ".bss", 5);
		bytes += 5;
	}
	Elf64_Word nameNameOffset = bytes - namesStart;
	memcpy(bytes, ".shstrtab", 10);
	bytes += 10;

	const Elf64_Xword namesSize = bytes - namesStart;

	if((bytes - bytesStart) % alignof(Elf64_Shdr) != 0)
	{
		memset(bytes, 0, alignof(Elf64_Shdr) - (bytes - bytesStart) % alignof(Elf64_Shdr));
		bytes += alignof(Elf64_Shdr) - (bytes - bytesStart) % alignof(Elf64_Shdr);
	}

	*(Elf64_Shdr*)bytes = (Elf64_Shdr){};
	bytes += sizeof(Elf64_Shdr);

	if(data->textSize > 0)
	{
		*(Elf64_Shdr*)bytes = (Elf64_Shdr){
			.sh_name = textNameOffset,
			.sh_type = SHT_PROGBITS,
			.sh_flags = SHF_ALLOC | SHF_EXECINSTR,
			.sh_offset = textOffset,
			.sh_size = data->textSize,
			.sh_addralign = 4
		};
		bytes += sizeof(Elf64_Shdr);
	}
	if(data->dataSize > 0)
	{
		*(Elf64_Shdr*)bytes = (Elf64_Shdr){
			.sh_name = dataNameOffset,
			.sh_type = SHT_PROGBITS,
			.sh_flags = SHF_ALLOC | SHF_WRITE,
			.sh_offset = dataOffset,
			.sh_size = data->dataSize,
			.sh_addralign = data->dataAlignment
		};
		bytes += sizeof(Elf64_Shdr);
	}
	if(data->bssSize > 0)
	{
		*(Elf64_Shdr*)bytes = (Elf64_Shdr){
			.sh_name = bssNameOffset,
			.sh_type = SHT_NOBITS,
			.sh_flags = SHF_ALLOC | SHF_WRITE,
			.sh_offset = bssOffset,
			.sh_size = data->bssSize,
			.sh_addralign = data->bssAlignment
		};
		bytes += sizeof(Elf64_Shdr);
	}

	*(Elf64_Shdr*)bytes = (Elf64_Shdr){
		.sh_name = nameNameOffset,
		.sh_type = SHT_STRTAB,
		.sh_offset = namesOffset,
		.sh_size = namesSize,
		.sh_addralign = 1
	};

	FILE* const file = fopen(fileName, "wb");
	if(!file)
	{
		free(bytesStart);
		return CYMB_OUT_OF_MEMORY;
	}

	if(fwrite(bytesStart, 1, size, file) != size)
	{
		free(bytesStart);
		fclose(file);
		return CYMB_OUT_OF_MEMORY;
	}

	free(bytesStart);

	if(fclose(file) != 0)
	{
		return CYMB_OUT_OF_MEMORY;
	}

	return result;
}

static CymbResult cymbElfRead(CymbElfFile* const file, CymbDiagnosticList* const diagnostics)
{
	(void)diagnostics;

	if(file->size < sizeof(Elf64_Ehdr) + sizeof(file->sections[SHN_UNDEF]))
	{
		return CYMB_INVALID;
	}
	const Elf64_Ehdr* const header = (const Elf64_Ehdr*)file->bytes;
	if(
		header->e_ident[EI_MAG0] != ELFMAG0 ||
		header->e_ident[EI_MAG1] != ELFMAG1 ||
		header->e_ident[EI_MAG2] != ELFMAG2 ||
		header->e_ident[EI_MAG3] != ELFMAG3 ||
		header->e_ident[EI_CLASS] != ELFCLASS64 ||
		header->e_ident[EI_DATA] != ELFDATA2LSB ||
		header->e_ident[EI_VERSION] != EV_CURRENT ||
		header->e_type != ET_REL ||
		header->e_machine != EM_AARCH64 ||
		header->e_version != EV_CURRENT ||
		header->e_shoff < sizeof(*header) || header->e_shoff > file->size - sizeof(file->sections[SHN_UNDEF])||
		header->e_ehsize != sizeof(*header) ||
		header->e_shentsize != sizeof(file->sections[SHN_UNDEF]) ||
		header->e_shnum >= SHN_LORESERVE ||
		header->e_shstrndx >= SHN_LORESERVE
	)
	{
		return CYMB_INVALID;
	}

	file->sections = (Elf64_Shdr*)(file->bytes + header->e_shoff);
	file->sectionCount = header->e_shnum == 0 ? file->sections[SHN_UNDEF].sh_size : header->e_shnum;
	if(file->sectionCount == 0 || header->e_shnum > (file->size - header->e_shoff) / header->e_shentsize)
	{
		return CYMB_INVALID;
	}
	const Elf64_Word sectionHeaderNamesIndex = header->e_shstrndx == SHN_XINDEX ? file->sections[SHN_UNDEF].sh_link : header->e_shstrndx;
	if(sectionHeaderNamesIndex >= file->sectionCount)
	{
		return CYMB_INVALID;
	}

	const Elf64_Shdr firstSectionHeader = {
		.sh_size = header->e_shnum == 0 ? file->sectionCount : 0,
		.sh_link = header->e_shstrndx == SHN_XINDEX ? sectionHeaderNamesIndex : 0
	};
	if(memcmp(&file->sections[SHN_UNDEF], &firstSectionHeader, sizeof(firstSectionHeader)) != 0)
	{
		return CYMB_INVALID;
	}

	if(
		sectionHeaderNamesIndex != SHN_UNDEF && (
		file->sections[sectionHeaderNamesIndex].sh_type != SHT_STRTAB ||
		file->sections[sectionHeaderNamesIndex].sh_flags != 0 ||
		file->sections[sectionHeaderNamesIndex].sh_size > file->size || file->sections[sectionHeaderNamesIndex].sh_offset > file->size - file->sections[sectionHeaderNamesIndex].sh_size ||
		*(file->bytes + file->sections[sectionHeaderNamesIndex].sh_offset) != '\0' ||
		*(file->bytes + file->sections[sectionHeaderNamesIndex].sh_offset + file->sections[sectionHeaderNamesIndex].sh_size - 1) != '\0' ||
		strcmp((const char*)(file->bytes + file->sections[sectionHeaderNamesIndex].sh_offset + file->sections[sectionHeaderNamesIndex].sh_name), ".shstrtab") != 0
	))
	{
		return CYMB_INVALID;
	}

	bool hasSymbolSection = false;
	for(Elf64_Xword sectionIndex = 0; sectionIndex < file->sectionCount; ++sectionIndex)
	{
		const Elf64_Shdr* const section = &file->sections[sectionIndex];

		if(section->sh_type != SHT_NOBITS && (section->sh_size > file->size || section->sh_offset > file->size - section->sh_size))
		{
			return CYMB_INVALID;
		}

		if(section->sh_type == SHT_NULL && sectionIndex != SHN_UNDEF)
		{
			return CYMB_INVALID;
		}

		if(
			section->sh_type == SHT_STRTAB && (
			*(file->bytes + section->sh_offset) != '\0' ||
			*(file->bytes + section->sh_offset + section->sh_size - 1) != '\0'
		))
		{
			return CYMB_INVALID;
		}

		if(section->sh_type == SHT_SYMTAB)
		{
			if(hasSymbolSection)
			{
				return CYMB_INVALID;
			}

			hasSymbolSection = true;

			const Elf64_Sym firstSymbol = {};
			if(
				section->sh_entsize != sizeof(Elf64_Sym) ||
				section->sh_size % section->sh_entsize != 0 ||
				section->sh_size == 0 ||
				section->sh_link >= file->sectionCount ||
				file->sections[section->sh_link].sh_type != SHT_STRTAB ||
				memcmp(file->bytes + section->sh_offset, &firstSymbol, sizeof(firstSymbol)) != 0
			)
			{
				return CYMB_INVALID;
			}
		}

		if(
			section->sh_type == SHT_RELA && (
			section->sh_entsize != sizeof(Elf64_Rela) ||
			section->sh_size % section->sh_entsize != 0 ||
			section->sh_link >= file->sectionCount ||
			file->sections[section->sh_link].sh_type != SHT_SYMTAB ||
			section->sh_info == SHN_UNDEF || section->sh_info >= file->sectionCount ||
			file->sections[section->sh_info].sh_size == 0
		))
		{
			return CYMB_INVALID;
		}

		if(
			section->sh_type == SHT_REL && (
			section->sh_entsize != sizeof(Elf64_Rel) ||
			section->sh_size % section->sh_entsize != 0 ||
			section->sh_link >= file->sectionCount ||
			file->sections[section->sh_link].sh_type != SHT_SYMTAB ||
			section->sh_info == SHN_UNDEF || section->sh_info >= file->sectionCount ||
			file->sections[section->sh_info].sh_size == 0
		))
		{
			return CYMB_INVALID;
		}

		const char* const name = (const char*)(file->bytes + file->sections[sectionHeaderNamesIndex].sh_offset + section->sh_name);

		if(strcmp(name, ".bss") == 0)
		{
			if(section->sh_type != SHT_NOBITS || section->sh_flags != (SHF_WRITE | SHF_ALLOC))
			{
				return CYMB_INVALID;
			}
		}

		if(strcmp(name, ".data") == 0)
		{
			if(section->sh_type != SHT_PROGBITS || section->sh_flags != (SHF_WRITE | SHF_ALLOC))
			{
				return CYMB_INVALID;
			}
		}

		if(strcmp(name, ".rodata") == 0)
		{
			if(section->sh_type != SHT_PROGBITS || section->sh_flags != SHF_ALLOC)
			{
				return CYMB_INVALID;
			}
		}

		if(strcmp(name, ".strtab") == 0)
		{
			if(section->sh_type != SHT_STRTAB || section->sh_flags != 0)
			{
				return CYMB_INVALID;
			}
		}

		if(strcmp(name, ".symtab") == 0)
		{
			if(section->sh_type != SHT_SYMTAB || section->sh_flags != 0)
			{
				return CYMB_INVALID;
			}
		}

		if(strcmp(name, ".text") == 0)
		{
			if(section->sh_type != SHT_PROGBITS || section->sh_flags != (SHF_ALLOC | SHF_EXECINSTR))
			{
				return CYMB_INVALID;
			}
		}
	}

	return CYMB_SUCCESS;
}

CymbResult cymbLink(const char* const* const fileNames, const size_t count, CymbDiagnosticList* const diagnostics)
{
	CymbResult result = CYMB_SUCCESS;

	CymbArena arena;
	cymbArenaCreate(&arena);

	CymbElfFile* const files = cymbArenaAllocate(&arena, count * sizeof(files[0]), alignof(typeof(files[0])));
	if(!files)
	{
		goto end;
	}

	for(size_t fileIndex = 0; fileIndex < count; ++fileIndex)
	{
		FILE* const file = fopen(fileNames[fileIndex], "rb");
		if(!file)
		{
			result = CYMB_FILE_NOT_FOUND;
			goto end;
		}

		if(fseek(file, 0, SEEK_END) != 0)
		{
			result = CYMB_OUT_OF_MEMORY;
			fclose(file);
			goto end;
		}

		const long sizeLong = ftell(file);
		if(sizeLong <= 0 || (unsigned long)sizeLong >= cymbSizeMax)
		{
			result = CYMB_OUT_OF_MEMORY;
			fclose(file);
			goto end;
		}

		files[fileIndex].size = sizeLong;
		files[fileIndex].bytes = cymbArenaAllocate(&arena, files[fileIndex].size, 1);
		if(!files[fileIndex].bytes)
		{
			result = CYMB_OUT_OF_MEMORY;
			fclose(file);
			goto end;
		}

		rewind(file);

		if(fread(files[fileIndex].bytes, 1, files[fileIndex].size, file) != files[fileIndex].size)
		{
			result = CYMB_OUT_OF_MEMORY;
			fclose(file);
			goto end;
		}

		if(fclose(file) != 0)
		{
			result = CYMB_OUT_OF_MEMORY;
			goto end;
		}

		result = cymbElfRead(&files[fileIndex], diagnostics);
		if(result != CYMB_SUCCESS)
		{
			goto end;
		}
	}

	end:
	cymbArenaFree(&arena);

	return result;
}
