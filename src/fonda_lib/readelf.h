#ifndef FONDA_LIB_READELF_H
#define FONDA_LIB_READELF_H

#include "lineinfo.h"

namespace fonda
{
// ----------------------------------------------------------------------------
struct elf_section
{
	uint32_t 		section_id;
	std::string		name_string;	// e.g. ".debug_info"
	uint64_t 		offset;			// position of the section in the elf file
	uint64_t 		size;			// size in bytes
	uint32_t 		type;			// one of SHT_*
	uint64_t 		flags;			// bitfield of SHF_*
	uint64_t 		addr;			// address offset
};


// ----------------------------------------------------------------------------
struct elf_symbol
{
	// Data derived from elf structure
	uint32_t st_name;			// offset in relevant string section
	uint8_t	st_info;
	uint8_t	st_other;
	uint16_t st_shndx;			// Associated section index
	uint64_t st_value;
	uint64_t st_size;

	// Additional data
	std::string name;
	std::string section_type;
};

// ----------------------------------------------------------------------------
struct elf_results
{
	std::vector<elf_section>		sections;
	std::vector<compilation_unit>	line_info_units;
	std::vector<elf_symbol>			symbols;
};

// ----------------------------------------------------------------------------
namespace elf_error
{
	enum
	{
		OK = 0,
		ERROR_READ_FILE = 1,						// Unable to read data from file
		ERROR_HEADER_MAGIC_FAIL = 2,				// Missing "ELF" signature
		ERROR_ELF_VERSION = 3,						// Unsupported version number
		ERROR_UNKNOWN_CLASS = 4,					// Neither LSB or MSB mode in header
		ERROR_INVALID_SECTION = 5,					// Tried to access an invalid section number
		ERROR_DWARF_VERSION_TOO_NEW = 6,			// DWARF version is greater than 5

		ERROR_DWARF_UNKNOWN_OPCODE = 1000,			// Dwarf main opcode not recognised
		ERROR_DWARF_UNKNOWN_EXTENDED_OPCODE = 1001,	// Dwarf extended opcode not recognised
		ERROR_DWARF_DEBUGLINE_PARSE = 1002,			// Over-read expected bounds of .debug_line data
		ERROR_DWARF_UNKNOWN_CONTENT_FORM = 1003,	// One of DW_FORM not supported
		ERROR_DWARF_UNKNOWN_CONTENT_TYPE = 1004,	// One of DW_TYPE not supported
	};
}

// ----------------------------------------------------------------------------
extern int process_elf_file(FILE* file, elf_results& output);

}
#endif
