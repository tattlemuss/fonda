#ifndef FONDA_LIB_LINEINFO_H
#define FONDA_LIB_LINEINFO_H

// Shared structures to describe line information in an executable.
#include <stdint.h>
#include <vector>
#include <string>

namespace fonda
{
// ----------------------------------------------------------------------------
// A single correlation between a machine instruction and position in a code file
struct code_point
{
	uint64_t address;
	uint16_t file_index;		// index within compilation_unit::files
	uint16_t column;
	uint32_t line;
};

// ----------------------------------------------------------------------------
// An instance of compiled code, which may be generated from multiple code files.
struct compilation_unit
{
	// A source file used when compiling this unit.
	struct file
	{
		size_t dir_index;		// index in compilation_unit::dirs
		uint64_t timestamp;		// optional time. May be be 0 if unknown.
		uint64_t length;		// length of source file in bytes. May be 0 when unknown.
		std::string path;		// original path as supplied to the compiler.
	};

	std::vector<std::string> dirs;
	std::vector<file> files;
	std::vector<code_point> points;
};

}
#endif // FONDA_LIB_LINEINFO_H
