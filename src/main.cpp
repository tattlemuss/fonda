#include <stdio.h>
#include <string>
#include <cstring>

#include "fonda_lib/readelf.h"

// ----------------------------------------------------------------------------
void usage()
{
	fprintf(stdout,
		"Usage: fonda <input_filename>\n\n"
	);
}

const char* title = "fonda v0.0\n";

// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{
	printf("%s\n", title);
	if (argc < 2)
	{
		fprintf(stderr, "Error: No filename\n");
		usage();
		return 1;
	}

	const int last_arg = argc - 1;							// last arg is reserved for filename
	for (int opt = 1; opt < last_arg; ++opt)
	{
		if (false)
		{
		}
		else
		{
			fprintf(stderr, "Error: Unknown option: '%s'\n", argv[opt]);
			usage();
			return 1;
		}
	}

	const char* fname = argv[argc - 1];
	FILE* pInfile = fopen(fname, "rb");
	if (!pInfile)
	{
		fprintf(stderr, "Error: Can't open file: %s\n", fname);
		return 1;
	}

	printf("File: \"%s\"\n", fname);

	int ret = 0;
	fonda::elf_results results;
	ret = fonda::process_elf_file(pInfile, results);
	fclose(pInfile);
	if (ret)
	{
		fprintf(stderr, "Parsing failed with error %d\n", ret);
		return 2;
	}

	// Dump output
	printf("\n==== SECTION INFORMATION ===\n\n");
	for (const fonda::elf_section& s : results.sections)
	{
		printf("[%03d] [%20s] [%08lx] [%08lx] [type: %08x] [addr:%08lx]\n", s.section_id, s.name_string.c_str(),
			s.offset, s.size, s.type, s.addr);
	}

	printf("\n\n==== LINE INFORMATION ===\n\n");
	for (const fonda::compilation_unit& unit : results.line_info_units)
	{
		for (const fonda::compilation_unit::file& file : unit.files)
		{
			printf("\tfile %s/%s\n",
				unit.dirs[file.dir_index].c_str(), file.path.c_str());
		}
		for (const fonda::code_point& cp : unit.points)
		{
			const fonda::compilation_unit::file& file = unit.files[cp.file_index];
			printf("\t\tAddress: %lx File: \"%s/%s\" Line: %d Col: %u\n", 
				cp.address,
				unit.dirs[file.dir_index].c_str(),
				file.path.c_str(),
				cp.line, cp.column);
		}
	}

	printf("\n\n==== SYMBOL INFORMATION ===\n\n");
	for (const fonda::elf_symbol& sym : results.symbols)
		printf("Symbol: %08lx (size: %08lx) binding=%x type=%x section=(%d, name %s) \"%s\"\n",
					sym.st_value, sym.st_size, sym.st_other >> 4, sym.st_other & 0xf,
					sym.st_shndx, sym.section_type.c_str(), sym.name.c_str());
	return ret;
}
